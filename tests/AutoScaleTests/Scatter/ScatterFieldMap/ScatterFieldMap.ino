/**
 * ScatterFieldMap.ino
 * @date 03-08-26
 *
 * ViewPoint Auto-Scale Test: Scatter — Migrating Sensor Clusters
 *
 * Simulates a 2D sensor field where three independent data sources
 * produce clustered XY measurements. Each cluster has its own center,
 * spread, and drift behavior. The auto-scaler must encompass all
 * active clusters simultaneously.
 *
 * Scenarios that stress auto-scale:
 * - All clusters near origin: tight scale
 * - One cluster migrates far from origin: scale must expand
 * - Cluster spread changes: tight gaussian → wide scatter
 * - A cluster temporarily disappears then reappears far away
 * - Two clusters converge then violently separate
 *
 * Real-world analogy: GPS multipath scatter, particle tracking,
 * multi-target radar returns, or industrial process control charts
 * where different process variables correlate in XY space.
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch scale response when a cluster jumps to a far quadrant
 * - Note how scale contracts when all clusters reconverge near origin
 * - Try different trace colors to track which cluster drives the scale
 */

#include <ViewPoint.h>
using namespace viewpoint;

// Cluster state
struct Cluster {
    float cx, cy;           // Current center
    float targetCx, targetCy;  // Target center (drifts toward this)
    float spread;           // Current gaussian spread (std dev)
    float targetSpread;
    float driftRate;
    bool active;
    unsigned long nextMigration;
};

Cluster clusters[3];

unsigned long sampleCount = 0;

void initCluster(Cluster& c, int idx);
void updateCluster(Cluster& c, unsigned long now);
void emitPoint(const char* label, Cluster& c);

void setup() {
    randomSeed(analogRead(0) ^ (analogRead(1) << 8));
    view.begin(PlotType::Scatter, Mode::Continuous);
    view.setDelay(2);

    view.addHorizontalReferenceLine(0.0);
    view.addVerticalReferenceLine(0.0);

    view.setAxisLabels("X", "Y");
    view.setPlotTitle("Field Map Test");

    view.trace("Cluster A").setColor(colors::Crimson);
    view.trace("Cluster B").setColor(colors::LimeGreen);
    view.trace("Cluster C").setColor(colors::DodgerBlue);

    unsigned long now = millis();
    for (int i = 0; i < 3; i++) {
        initCluster(clusters[i], i);
        clusters[i].nextMigration = now + 2000 + random(3000);
    }
}

void loop() {
    unsigned long now = millis();

    for (int i = 0; i < 3; i++) {
        updateCluster(clusters[i], now);
    }

    // Emit one point per cluster per sample
    if (clusters[0].active) emitPoint("Cluster A", clusters[0]);
    if (clusters[1].active) emitPoint("Cluster B", clusters[1]);
    if (clusters[2].active) emitPoint("Cluster C", clusters[2]);

    view.send();
    sampleCount++;
}

void initCluster(Cluster& c, int idx) {
    c.cx = (random(200) - 100) / 10.0;
    c.cy = (random(200) - 100) / 10.0;
    c.targetCx = c.cx;
    c.targetCy = c.cy;
    c.spread = 1.0 + random(300) / 100.0;
    c.targetSpread = c.spread;
    c.driftRate = 0.005 + random(20) / 1000.0;
    c.active = true;
}

void updateCluster(Cluster& c, unsigned long now) {
    // Drift toward target
    c.cx += (c.targetCx - c.cx) * c.driftRate;
    c.cy += (c.targetCy - c.cy) * c.driftRate;
    c.spread += (c.targetSpread - c.spread) * 0.01;

    // Time for a migration event?
    if (now >= c.nextMigration) {
        int event = random(8);
        switch (event) {
            case 0:
                // Near origin
                c.targetCx = (random(200) - 100) / 50.0;
                c.targetCy = (random(200) - 100) / 50.0;
                c.targetSpread = 0.5 + random(200) / 100.0;
                c.driftRate = 0.01;
                break;
            case 1:
                // Moderate distance
                c.targetCx = (random(2) == 0 ? 1 : -1) * (20.0 + random(80));
                c.targetCy = (random(2) == 0 ? 1 : -1) * (20.0 + random(80));
                c.targetSpread = 2.0 + random(500) / 100.0;
                c.driftRate = 0.005;
                break;
            case 2:
                // Far from origin (scale expansion)
                c.targetCx = (random(2) == 0 ? 1 : -1) * (200.0 + random(800));
                c.targetCy = (random(2) == 0 ? 1 : -1) * (200.0 + random(800));
                c.targetSpread = 5.0 + random(2000) / 100.0;
                c.driftRate = 0.003;
                break;
            case 3:
                // Instant snap to new location
                c.cx = (random(2) == 0 ? 1 : -1) * (50.0 + random(450));
                c.cy = (random(2) == 0 ? 1 : -1) * (50.0 + random(450));
                c.targetCx = c.cx;
                c.targetCy = c.cy;
                c.targetSpread = 1.0 + random(1000) / 100.0;
                break;
            case 4:
                // Tighten spread (precision mode)
                c.targetSpread = 0.1 + random(50) / 100.0;
                break;
            case 5:
                // Explode spread
                c.targetSpread = 20.0 + random(80);
                break;
            case 6:
                // Deactivate briefly
                c.active = false;
                break;
            case 7:
                // Reactivate at random location
                c.active = true;
                c.cx = (random(2000) - 1000) / 2.0;
                c.cy = (random(2000) - 1000) / 2.0;
                c.targetCx = c.cx;
                c.targetCy = c.cy;
                c.targetSpread = 3.0 + random(700) / 100.0;
                break;
        }

        c.nextMigration = now + 2000 + random(6000);
    }
}

void emitPoint(const char* label, Cluster& c) {
    // Box-Muller-like approximation: sum of uniforms
    float dx = 0, dy = 0;
    for (int i = 0; i < 4; i++) {
        dx += (random(2000) - 1000) / 1000.0;
        dy += (random(2000) - 1000) / 1000.0;
    }
    dx *= c.spread * 0.5;
    dy *= c.spread * 0.5;

    float x = c.cx + dx;
    float y = c.cy + dy;

    view.addData(label, constrain(x, -1e6f, 1e6f), constrain(y, -1e6f, 1e6f));
}
