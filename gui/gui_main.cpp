#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include "ReliefManager.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>


struct RoadView { int a; int b; double cost; bool blocked; };

struct App {
    std::unique_ptr<ReliefManager> relief;
    std::map<int, ImVec2> pos;          // node id -> map position (0..1), draggable
    std::map<int, int> maxStock;        // highest stock seen per center, for the bars
    std::vector<std::string> log;
    std::vector<float> histX, histY;    // unmet demand per step, for the chart
    int stepNo = 0;
    bool autoRun = false;
    float interval = 0.2f;
    float speed = 10.0f;
    double lastTick = 0.0;
};

struct Ranking {
    std::vector<RankedZone> list;
    std::map<int, float> score;
};

// >>> STRUCTS
static int totalUnmet(const ReliefManager& r) {
    int total = 0;
    for (const auto& e : r.allZones()) total += e.second.unmetDemand;
    return total;
}

// Every road once (a < b), with its blocked flag, read straight from the engine.
static std::vector<RoadView> collectRoads(const ReliefManager& r) {
    std::vector<RoadView> out;
    for (const auto& node : r.roadNetwork().adjacency()) {
        for (const Road& road : node.second) {
            if (node.first < road.to) out.push_back({node.first, road.to, road.cost, road.blocked});
        }
    }
    std::sort(out.begin(), out.end(), [](const RoadView& x, const RoadView& y) {
        return x.a != y.a ? x.a < y.a : x.b < y.b;
    });
    return out;
}

static void loadDemo(App& a) {
    a.relief = std::make_unique<ReliefManager>();
    a.pos.clear();
    a.maxStock.clear();
    a.log.clear();
    a.histX.clear();
    a.histY.clear();
    a.stepNo = 0;
    a.autoRun = false;

    ReliefManager& r = *a.relief;
    r.setTravelSpeed(a.speed);

    //            id  name            severity population demand
    r.addZone({1, "North Sector",   5, 1200, 500});
    r.addZone({2, "River District", 4, 2000, 300});
    r.addZone({3, "East Village",   3, 800,  700});

    ResourceCenter hub(101, "Central Warehouse");
    hub.addStock(DEFAULT_SUPPLY_ID, 800);
    r.addCenter(hub);

    ResourceCenter depot(102, "Harbor Depot");
    depot.addStock(DEFAULT_SUPPLY_ID, 400);
    r.addCenter(depot);

    r.addLocation(201);   // junction
    r.addRoad(101, 201, 4);
    r.addRoad(201, 1, 6);
    r.addRoad(201, 2, 3);
    r.addRoad(201, 3, 9);
    r.addRoad(102, 3, 5);
    r.addRoad(102, 2, 7);

    r.addVehicle(Vehicle(1, 100, 101));
    r.addVehicle(Vehicle(2, 60, 101));
    r.addVehicle(Vehicle(3, 80, 102));

    a.pos[1]   = ImVec2(0.20f, 0.15f);
    a.pos[2]   = ImVec2(0.72f, 0.28f);
    a.pos[3]   = ImVec2(0.78f, 0.88f);
    a.pos[101] = ImVec2(0.20f, 0.60f);
    a.pos[102] = ImVec2(0.38f, 0.92f);
    a.pos[201] = ImVec2(0.45f, 0.40f);

    a.histX.push_back(0.0f);
    a.histY.push_back(static_cast<float>(totalUnmet(r)));
}

static ImU32 heat(float t) {              // green -> amber -> red
    t = std::clamp(t, 0.0f, 1.0f);
    float red = t < 0.5f ? t * 2.0f : 1.0f;
    float green = t < 0.5f ? 1.0f : (1.0f - t) * 2.0f;
    return IM_COL32(static_cast<int>(red * 235), static_cast<int>(green * 200), 60, 255);
}

static void placeWindow(float x, float y, float w, float h) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + x * vp->WorkSize.x,
                                   vp->WorkPos.y + y * vp->WorkSize.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * vp->WorkSize.x, h * vp->WorkSize.y), ImGuiCond_FirstUseEver);
}

// Runs one engine step and captures what the engine prints into the event log.
static void stepOnce(App& a) {
    std::ostringstream captured;
    std::streambuf* old = std::cout.rdbuf(captured.rdbuf());
    bool progressed = false;
    try {
        progressed = a.relief->runStep();
    } catch (const std::exception& e) {
        captured << "ERROR: " << e.what() << "\n";
    }
    std::cout.rdbuf(old);

    a.stepNo++;
    a.log.push_back("=== Step " + std::to_string(a.stepNo) + " ===");
    std::istringstream in(captured.str());
    std::string line;
    while (std::getline(in, line)) a.log.push_back(line);

    a.histX.push_back(static_cast<float>(a.stepNo));
    a.histY.push_back(static_cast<float>(totalUnmet(*a.relief)));

    if (!progressed) a.autoRun = false;   // nothing left to do
}

static void drawControls(App& a) {
    placeWindow(0.0f, 0.0f, 0.22f, 0.28f);
    ImGui::Begin("Control");

    if (ImGui::Button("Step")) stepOnce(a);
    ImGui::SameLine();
    if (ImGui::Button(a.autoRun ? "Pause" : "Run")) {
        a.autoRun = !a.autoRun;
        a.lastTick = glfwGetTime();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset demo")) {
        loadDemo(a);
        ImGui::End();
        return;
    }

    ImGui::SliderFloat("Step interval (s)", &a.interval, 0.1f, 3.0f);
    if (ImGui::SliderFloat("Travel speed", &a.speed, 0.0f, 40.0f, "%.0f")) {
        a.relief->setTravelSpeed(a.speed);   // 0 = instant deliveries
    }

    ImGui::Separator();
    ImGui::Text("Step %d", a.stepNo);
    ImGui::Text("Unmet demand: %d units", totalUnmet(*a.relief));
    ImGui::Text("Vehicles on the road: %s", a.relief->hasVehiclesOnTrip() ? "yes" : "no");
    ImGui::End();
}

static void drawLog(App& a) {
    placeWindow(0.22f, 0.65f, 0.78f, 0.35f);
    ImGui::Begin("Event log");
    if (ImGui::Button("Clear")) a.log.clear();
    ImGui::BeginChild("logscroll");
    for (const std::string& line : a.log) {
        ImVec4 col(0.85f, 0.9f, 0.95f, 1.0f);
        if (line.rfind("===", 0) == 0) col = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
        else if (line.find("Shortfall") != std::string::npos ||
                 line.find("UNREACHABLE") != std::string::npos ||
                 line.find("NO STOCK") != std::string::npos) col = ImVec4(0.95f, 0.45f, 0.4f, 1.0f);
        else if (line.find("delivered") != std::string::npos) col = ImVec4(0.45f, 0.9f, 0.55f, 1.0f);
        ImGui::TextColored(col, "%s", line.c_str());
    }
    static size_t lastCount = 0;
    if (a.log.size() != lastCount) {
        ImGui::SetScrollHereY(1.0f);
        lastCount = a.log.size();
    }
    ImGui::EndChild();
    ImGui::End();
}

static Ranking computeRanking(const ReliefManager& r) {
    Ranking out;
    std::vector<DisasterZone> active;
    for (const auto& e : r.allZones()) {
        if (e.second.unmetDemand > 0) active.push_back(e.second);
    }
    if (active.empty()) return out;
    try {
        PriorityManager pm;
        out.list = pm.rankZones(active);
        for (const auto& rz : out.list) out.score[rz.zone.id] = static_cast<float>(rz.score);
    } catch (const std::exception&) {
        out.list.clear();
        out.score.clear();
    }
    return out;
}

static void drawPriority(const Ranking& rk) {
    placeWindow(0.0f, 0.28f, 0.22f, 0.34f);
    ImGui::Begin("Priority queue");
    if (rk.list.empty()) {
        ImGui::TextDisabled("No unmet demand.");
    } else if (ImGui::BeginTable("prio", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("#");
        ImGui::TableSetupColumn("Zone");
        ImGui::TableSetupColumn("Score");
        ImGui::TableSetupColumn("Unmet");
        ImGui::TableHeadersRow();
        int rank = 1;
        for (const auto& rz : rk.list) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", rank++);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(rz.zone.name.c_str());
            ImGui::TableNextColumn();
            float s = std::clamp(static_cast<float>(rz.score), 0.0f, 1.0f);
            char buf[16];
            std::snprintf(buf, sizeof buf, "%.3f", rz.score);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::ColorConvertU32ToFloat4(heat(s)));
            ImGui::ProgressBar(s, ImVec2(-1.0f, 0.0f), buf);
            ImGui::PopStyleColor();
            ImGui::TableNextColumn(); ImGui::Text("%d", rz.zone.unmetDemand);
        }
        ImGui::EndTable();
    }
    ImGui::TextDisabled("Scoring weights are engine defaults.");
    ImGui::End();
}

static void drawInventory(App& a) {
    placeWindow(0.0f, 0.62f, 0.22f, 0.38f);
    ImGui::Begin("Inventory");
    for (const auto& e : a.relief->allCenters()) {
        const ResourceCenter& c = e.second;
        const int stock = c.availableStock(DEFAULT_SUPPLY_ID);
        int& mx = a.maxStock[c.id()];
        mx = std::max(mx, stock);
        const float f = mx > 0 ? static_cast<float>(stock) / static_cast<float>(mx) : 0.0f;

        ImGui::TextUnformatted(c.name().c_str());
        char buf[32];
        std::snprintf(buf, sizeof buf, "%d / %d", stock, mx);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                              f < 0.2f ? ImVec4(0.9f, 0.25f, 0.2f, 1.0f) : ImVec4(0.25f, 0.6f, 0.95f, 1.0f));
        ImGui::ProgressBar(f, ImVec2(-1.0f, 0.0f), buf);
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }
    ImGui::End();
}

static void drawVehicles(App& a) {
    placeWindow(0.75f, 0.0f, 0.25f, 0.30f);
    ImGui::Begin("Vehicles");
    if (ImGui::BeginTable("veh", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Cap");
        ImGui::TableSetupColumn("Home");
        ImGui::TableSetupColumn("State");
        ImGui::TableSetupColumn("");
        ImGui::TableHeadersRow();

        std::vector<int> ids;
        for (const auto& e : a.relief->allVehicles()) ids.push_back(e.first);
        std::sort(ids.begin(), ids.end());

        for (int id : ids) {
            const Vehicle& v = *a.relief->findVehicle(id);
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", v.id());
            ImGui::TableNextColumn(); ImGui::Text("%d", v.capacity());
            ImGui::TableNextColumn(); ImGui::Text("%d", v.location());
            ImGui::TableNextColumn();
            if (!v.isAvailable()) {
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "OFFLINE");
            } else if (v.isOnTrip()) {
                const Trip& t = v.trip();
                ImGui::Text("%s zone %d (%d left)", t.delivered ? "returning from" : "to", t.zoneId,
                            t.returnAt - t.elapsed);
            } else {
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 0.5f, 1.0f), "idle");
            }
            ImGui::TableNextColumn();
            const bool online = v.isAvailable();
            if (ImGui::SmallButton(((online ? "Off##v" : "On##v") + std::to_string(id)).c_str())) {
                a.relief->setVehicleAvailable(id, !online);
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

static void drawRoads(App& a) {
    placeWindow(0.75f, 0.30f, 0.25f, 0.18f);
    ImGui::Begin("Roads");
    if (ImGui::BeginTable("roads", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("From");
        ImGui::TableSetupColumn("To");
        ImGui::TableSetupColumn("Cost");
        ImGui::TableSetupColumn("");
        ImGui::TableHeadersRow();
        for (const RoadView& road : collectRoads(*a.relief)) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", road.a);
            ImGui::TableNextColumn(); ImGui::Text("%d", road.b);
            ImGui::TableNextColumn(); ImGui::Text("%.0f", road.cost);
            ImGui::TableNextColumn();
            const std::string tag = "##r" + std::to_string(road.a) + "_" + std::to_string(road.b);
            if (ImGui::SmallButton(((road.blocked ? "Open" : "Block") + tag).c_str())) {
                a.relief->setRoadBlocked(road.a, road.b, !road.blocked);
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

// Any node without a position gets a spot, so new scenarios still draw.
static void ensurePositions(App& a) {
    auto give = [&](int id) {
        if (a.pos.count(id) == 0) {
            a.pos[id] = ImVec2(0.1f + 0.8f * ((id * 37) % 100) / 100.0f,
                               0.1f + 0.8f * ((id * 53) % 100) / 100.0f);
        }
    };
    for (const auto& e : a.relief->allZones()) give(e.first);
    for (const auto& e : a.relief->allCenters()) give(e.first);
    for (const auto& n : a.relief->roadNetwork().adjacency()) give(n.first);
}

static void drawMap(App& a, const Ranking& rk) {
    placeWindow(0.22f, 0.0f, 0.53f, 0.65f);
    ImGui::Begin("Map", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                 ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    if (size.x < 140.0f || size.y < 140.0f) { ImGui::End(); return; }

    ReliefManager& r = *a.relief;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(10, 14, 20, 255));
    for (float x = origin.x; x < origin.x + size.x; x += 40.0f)
        dl->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + size.y), IM_COL32(255, 255, 255, 12));
    for (float y = origin.y; y < origin.y + size.y; y += 40.0f)
        dl->AddLine(ImVec2(origin.x, y), ImVec2(origin.x + size.x, y), IM_COL32(255, 255, 255, 12));

    const float pad = 60.0f;
    const float w = size.x - 2.0f * pad;
    const float h = size.y - 2.0f * pad;
    auto toScreen = [&](int id) {
        const ImVec2& p = a.pos[id];
        return ImVec2(origin.x + pad + p.x * w, origin.y + pad + p.y * h);
    };

    // roads (blocked ones are red)
    for (const RoadView& road : collectRoads(r)) {
        ImVec2 p = toScreen(road.a), q = toScreen(road.b);
        dl->AddLine(p, q, road.blocked ? IM_COL32(220, 70, 60, 255) : IM_COL32(90, 110, 130, 255),
                    road.blocked ? 3.0f : 2.0f);
        char buf[24];
        std::snprintf(buf, sizeof buf, road.blocked ? "%.0f BLOCKED" : "%.0f", road.cost);
        dl->AddText(ImVec2((p.x + q.x) * 0.5f + 4.0f, (p.y + q.y) * 0.5f + 2.0f),
                    road.blocked ? IM_COL32(240, 110, 100, 255) : IM_COL32(130, 150, 170, 255), buf);
    }

    int maxDemand = 1;
    for (const auto& e : r.allZones()) maxDemand = std::max(maxDemand, e.second.unmetDemand);

    // nodes: zones = circles sized by demand and coloured by score, centers = squares
    for (auto& kv : a.pos) {
        const int id = kv.first;
        ImVec2 c = toScreen(id);
        const DisasterZone* zone = r.findZone(id);
        const ResourceCenter* center = r.findCenter(id);

        float rad = 7.0f;
        std::string label = std::to_string(id);

        if (zone) {
            rad = 10.0f + 14.0f * static_cast<float>(zone->unmetDemand) / static_cast<float>(maxDemand);
            auto it = rk.score.find(id);
            ImU32 col = (zone->unmetDemand > 0 && it != rk.score.end())
                            ? heat(it->second) : IM_COL32(60, 170, 200, 255);   // blue = served
            dl->AddCircleFilled(c, rad, col);
            dl->AddCircle(c, rad + 2.0f, IM_COL32(255, 255, 255, 90), 0, 1.5f);
            label = zone->name + "  [" + std::to_string(zone->unmetDemand) + " needed]";
        } else if (center) {
            rad = 11.0f;
            dl->AddRectFilled(ImVec2(c.x - rad, c.y - rad), ImVec2(c.x + rad, c.y + rad),
                              IM_COL32(70, 140, 230, 255), 3.0f);
            label = center->name() + "  [stock " +
                    std::to_string(center->availableStock(DEFAULT_SUPPLY_ID)) + "]";
        } else {
            dl->AddCircleFilled(c, 5.0f, IM_COL32(140, 150, 160, 255));         // junction
        }
        dl->AddText(ImVec2(c.x + rad + 6.0f, c.y - 7.0f), IM_COL32(220, 230, 240, 255), label.c_str());

        // drag nodes around to tidy the layout
        ImGui::SetCursorScreenPos(ImVec2(c.x - rad, c.y - rad));
        ImGui::InvisibleButton(("node" + std::to_string(id)).c_str(), ImVec2(rad * 2.0f, rad * 2.0f));
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 d = ImGui::GetIO().MouseDelta;
            kv.second.x += d.x / w;
            kv.second.y += d.y / h;
        }
    }

    // vehicles on the road, placed along their trip path
    for (const auto& e : r.allVehicles()) {
        const Vehicle& v = e.second;
        if (!v.isOnTrip()) continue;
        const Trip& t = v.trip();
        if (t.path.size() < 2 || t.returnAt <= 0) continue;

        float f = std::min(1.0f, static_cast<float>(t.elapsed) / static_cast<float>(t.returnAt));
        float s = f * static_cast<float>(t.path.size() - 1);
        size_t i = std::min(static_cast<size_t>(s), t.path.size() - 2);
        float k = s - static_cast<float>(i);

        ImVec2 p = toScreen(t.path[i]), q = toScreen(t.path[i + 1]);
        ImVec2 m(p.x + (q.x - p.x) * k, p.y + (q.y - p.y) * k);
        dl->AddCircleFilled(m, 6.0f, t.delivered ? IM_COL32(120, 220, 140, 255)
                                                 : IM_COL32(255, 200, 60, 255));
        dl->AddText(ImVec2(m.x + 8.0f, m.y - 16.0f), IM_COL32(255, 255, 255, 255),
                    ("V" + std::to_string(v.id())).c_str());
    }

    dl->AddText(ImVec2(origin.x + 8.0f, origin.y + size.y - 20.0f), IM_COL32(150, 160, 170, 255),
                "colour = priority, size = unmet demand | squares = centers | "
                "yellow = delivering, green = returning | drag nodes");
    ImGui::End();
}

static void drawChart(const App& a) {
    placeWindow(0.75f, 0.48f, 0.25f, 0.17f);
    ImGui::Begin("Unmet demand");
    if (ImPlot::BeginPlot("Total unmet demand", ImVec2(-1.0f, -1.0f))) {
        ImPlot::SetupAxes("Step", "Units", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Unmet", a.histX.data(), a.histY.data(), static_cast<int>(a.histX.size()));
        ImPlot::EndPlot();
    }
    ImGui::End();
}
// >>> FUNCTIONS

int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(1600, 900, "Disaster Relief Control Room", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 3.0f;
    style.FrameRounding = 3.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.08f, 0.10f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.18f, 0.28f, 1.0f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    App app;
    loadDemo(app);

    // >>> BEFORE LOOP

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

    if (app.autoRun && glfwGetTime() - app.lastTick > app.interval) {
        stepOnce(app);
        app.lastTick = glfwGetTime();
    }

        // >>> AUTO-RUN

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        const Ranking ranking = computeRanking(*app.relief);
        ensurePositions(app);
        // >>> PER-FRAME

        // >>> DRAW
        drawControls(app);
        drawLog(app);
        drawPriority(ranking);
        drawInventory(app);
        drawVehicles(app);
        drawRoads(app);
        drawMap(app, ranking);
        drawChart(app);

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.05f, 0.06f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}