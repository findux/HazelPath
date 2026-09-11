#include "simulation/SimulationController.h"
#include "simulation/MotionInterpolator.h"
#include "simulation/ToolpathVisibility.h"
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>

static bool near(double a, double b) { return std::abs(a - b) < 1e-8; }

int main() {
    nc::Toolpath path;
    path.addToolChange(1, 0, "T1");
    path.addLinear({10, 0, 0}, 600, 1, "line"); // 1 second
    path.addDwell(1, 2, "dwell");
    path.addRapid({60, 0, 0}, 3, "rapid"); // 1 second
    path.addToolChange(2, 4, "T2");
    path.addLinear({70, 0, 0}, 600, 5, "line"); // 1 second
    nc::SimulationController sim;
    sim.setToolpath(&path);
    assert(near(sim.getProgress(), 0));
    assert(near(sim.getPosition().x, 0));
    sim.play();
    sim.update(.5);
    assert(near(sim.getPosition().x, 5));
    sim.pause();
    sim.update(10);
    assert(near(sim.getPosition().x, 5));
    for (double t : {0., .25, .5, 1., .5, .25, 0., .875, .125}) {
        sim.seek(t);
        nc::SimulationController forward;
        forward.setToolpath(&path);
        forward.play();
        forward.update(t * 4);
        assert(near(sim.getPosition().x, forward.getPosition().x));
        assert(near(sim.getProgress(), t));
    }
    // Per-command rapid/feed prefixes across dwell and tool changes.
    const nc::ToolpathVertexBoundary boundaries[] = {
        {0,0}, {0,0}, {2,0}, {2,0}, {2,2}, {2,2}, {4,2}
    };
    for (double p : {0., .125, .25, .5, .625, .75, .875, 1., .5, .25, 0.}) {
        sim.seek(p);
        const auto index = sim.getCommandIndex();
        const auto next = std::min(index + 1, path.size());
        const auto cut = nc::toolpathDrawRange(boundaries[index][0], boundaries[next][0], sim.getCommandProgress());
        const auto rapid = nc::toolpathDrawRange(boundaries[index][1], boundaries[next][1], sim.getCommandProgress());
        const double time = p * 4.;
        const double expectedCut = std::clamp(time, 0., 1.) + std::clamp(time - 3., 0., 1.);
        const double expectedRapid = std::clamp(time - 2., 0., 1.);
        assert(near(cut.completedVertices / 2. + cut.partialProgress, expectedCut));
        assert(near(rapid.completedVertices / 2. + rapid.partialProgress, expectedRapid));
    }
    sim.seekToCommand(5, 1); // same entry point as step forward
    assert(near(sim.getPosition().x, 70));
    sim.seekToCommand(1, 0); // step backward across operations
    assert(near(sim.getPosition().x, 0));
    sim.seek(.5);
    sim.setSpeed(-1);
    sim.play();
    sim.update(1);
    assert(near(sim.getProgress(), .25));
    sim.update(1);
    assert(near(sim.getProgress(), 0));
    sim.setSpeed(1);
    sim.seek(1);
    sim.play();
    assert(near(sim.getProgress(), 0));
    sim.update(4);
    assert(sim.getStatus() == nc::SimulationStatus::Finished);
    sim.stop();
    assert(near(sim.getProgress(), 0));

    // Existing tessellation: full chords plus one partial chord, reversible.
    for (double p : {0., .25, .5, 1., .5, .25, 0., .31}) {
        auto r = nc::toolpathDrawRange(20, 40, p);
        assert(r.completedVertices % 2 == 0);
        assert(near((r.completedVertices - 20) / 2. + r.partialProgress, 10 * p));
    }
    auto dwell = nc::toolpathDrawRange(20, 20, .5);
    assert(dwell.completedVertices == 20 && dwell.partialProgress == 0);
    auto linear = nc::toolpathDrawRange(0, 2, .5);
    assert(linear.completedVertices == 0 && near(linear.partialProgress, .5));

    nc::Toolpath arc;
    arc.setCurrentPosition({10, 0, 0});
    arc.addArc({0, 10, 2}, {0, 0, 0}, false, nc::ArcPlane::XY, 600, 0, "helix");
    sim.setToolpath(&arc);
    for (double p : {.1, .75, .25, 1., 0.}) {
        sim.seek(p);
        auto expected = nc::MotionInterpolator::sample(arc.getCommandRef(0), p);
        assert(near(sim.getPosition().x, expected.x));
        assert(near(sim.getPosition().z, expected.z));
    }
    nc::Toolpath large;
    for (int i = 1; i <= 200000; ++i) large.addLinear({double(i), 0, 0}, 60, i, "");
    sim.setToolpath(&large);
    const auto started = std::chrono::steady_clock::now();
    for (int i = 0; i < 10000; ++i) {
        const double p = ((i * 7919) % 10001) / 10000.;
        sim.seek(p);
        assert(near(sim.getProgress(), p));
        assert(std::abs(sim.getPosition().x - p * 200000) < 1e-7);
    }
    std::cout << "200000 commands, 10000 seeks: "
              << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count()
              << " ms\n";
    nc::Toolpath empty;
    sim.setToolpath(&empty);
    sim.play(); sim.seek(.5); sim.update(1);
    assert(near(sim.getProgress(), 0));
    std::cout << "Simulation and visibility checks passed\n";
}
