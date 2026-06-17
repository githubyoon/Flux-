// ============================================================
// Flux 6.0 — Skipped Functions Manual Test
// Run each section individually (some block / need internet)
// ============================================================

function main() {
    print("=== Manual Test for Skipped Functions ===");
    print("Run each section individually.");
    print("");

    // ── 1. gui ──
    print("--- gui (needs window) ---");
    print("Uncomment to test:");
    print("// gui.window(\"Test\", 400, 300);");

    print("// gui.msgbox(\"Hello from Flux\", \"Test\");");

    print("// gui.label(\"Hello\", 10, 10, 100, 30);");
    print("// gui.button(\"Click\", 10, 50, 100, 30, \"onClick\");");
    print("// gui.loop();");

    print("");
    
    // ── 2. math (pi, e) ──
    print("--- math (pi, e) — pre-existing stack bug ---");
    print("// float p = math.pi; print(\"pi = \" + p);");
    print("// float e = math.e; print(\"e = \" + e);");
    print("");

    // ── 3. console (input, readkey) ──
    print("--- console (input, readkey) — blocks for input ---");
    print("// string name = console.input(\"Enter name: \");");
    print("// print(\"Hello, \" + name);");
    print("");
    print("// string key = console.readkey();");
    print("// print(\"You pressed: \" + key);");
    print("");

    // ── 4. system (exit) ──
    print("--- system (exit) — terminates ---");
    print("// system.exit(0);");
    print("// print(\"This won't print\");");
    print("");

    // ── 5. net (get, post, put, del, download, ip) — need internet ──
    print("--- net (need internet) ---");
    print("// string r = net.get(\"http://example.com\");");
    print("// print(r);");
    print("");
    print("// string r2 = net.post(\"http://example.com\", \"data\");");
    print("// print(r2);");
    print("");
    print("// string r3 = net.put(\"http://example.com\", \"data\");");
    print("// print(r3);");
    print("");
    print("// string r4 = net.del(\"http://example.com\");");
    print("// print(r4);");
    print("");
    print("// net.download(\"http://example.com/index.html\", \"index.html\");");
    print("// print(\"downloaded\");");
    print("");
    print("// string ip = net.ip();");
    print("// print(\"my ip = \" + ip);");
    print("");

    print("=== Done ===");
}
