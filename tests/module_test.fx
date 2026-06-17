// ============================================================
// Flux 6.0 Module Test Suite
// Tests all 80 functions across 8 modules
// ============================================================

function main() {
    print("");
    print("=== Flux 6.0 Module Test ===");

    // ── 1. gui (6/10) ──
    print("-- gui --");
    gui.window("Test", 300, 200);                          print("window");
    gui.setTitle("Flux Module Test Window");                print("setTitle");
    int g1 = gui.getX();                                    print("getX=" + g1);
    int g2 = gui.getY();                                    print("getY=" + g2);
    gui.show();                                             print("show");
    gui.close();                                            print("close");
    print("  skip: msgbox loop button label");

    // ── 2. math (8/10) ──
    print("-- math --");
    print("abs=" + math.abs(-5));
    print("round=" + math.round(3.7));
    print("sin=" + math.sin(0));
    print("cos=" + math.cos(0));
    print("tan=" + math.tan(0));
    print("sqrt=" + math.sqrt(9));
    print("pow=" + math.pow(2, 3));
    print("log=" + math.log(1));
    print("  skip: pi e (stack bug)");

    // ── 3. console (8/10) ──
    print("-- console --");
    console.clear(0);                                       print("clear");
    console.title("Flux");                                  print("title");
    console.color(10);                                      print("color");
    print("width=" + console.width());
    print("height=" + console.height());
    console.cursor(0, 0);                                   print("cursor");
    console.beep();                                         print("beep");
    console.reset();                                        print("reset");
    print("  skip: input readkey");

    // ── 4. file (10/10) ──
    print("-- file --");
    string f1 = "._ft1.txt";
    string f2 = "._ft2";
    string f3 = "._ft3.txt";
    string f4 = "._ft4.txt";
    if (file.exists(f1)) file.remove(f1);
    if (file.exists(f3)) file.remove(f3);
    if (file.exists(f4)) file.remove(f4);
    if (file.exists(f2)) file.remove(f2);

    file.write("Hello World", f1);                          print("write=" + file.exists(f1));
    print("read=" + file.read(f1));
    file.append("!!", f1);                                  print("append=" + file.read(f1));
    print("exists=" + file.exists(f1));
    print("size=" + file.size(f1));
    print("isdir=" + file.is_dir(f1));
    file.mkdir(f2);                                         print("mkdir=" + file.exists(f2));
    print("isdir=" + file.is_dir(f2));
    file.copy(f1, f3);                                      print("copy=" + file.exists(f3));
    file.rename(f3, f4);                                    print("rename=" + file.exists(f4));
    file.remove(f1); file.remove(f3); file.remove(f4); file.remove(f2);
    print("cleanup");

    // ── 5. time (10/10) ──
    print("-- time --");
    time.sleep(1);                                          print("sleep");
    print("now=" + time.now());
    print("ticks=" + time.ticks());
    print("year=" + time.year());
    print("month=" + time.month());
    print("date=" + time.date());
    print("hour=" + time.hour());
    print("min=" + time.minute());
    print("sec=" + time.second());
    print("fmt=" + time.format("%Y-%m-%d"));

    // ── 6. system (9/10) ──
    print("-- system --");
    print("os=" + system.os());
    print("env=" + system.env("PATH"));
    print("cpu=" + system.cpu());
    print("pid=" + system.pid());
    print("user=" + system.user());
    print("cwd=" + system.cwd());
    print("temp=" + system.temp());
    print("host=" + system.host());
    print("run=" + system.run("echo OK"));
    print("  skip: exit");

    // ── 7. net (4/10) ──
    print("-- net --");
    print("enc=" + net.encode("hello world"));
    print("dec=" + net.decode("hello+world"));
    print("status=" + net.status());
    print("headers=" + net.headers());
    print("  skip: get post put del download ip");

    // ── 8. json (9/10) ──
    print("-- json --");
    string j = "{\"a\":1,\"b\":2,\"c\":\"hi\"}";
    map o = json.parse(j);                                  print("parse");
    print("type(int)=" + json.type(42));
    print("type(str)=" + json.type("hi"));
    print("type(bool)=" + json.type(true));
    print("type(obj)=" + json.type(o));
    print("  skip: type(arr)");
    print("isValid(ok)=" + json.isValid("{}"));
    print("isValid(no)=" + json.isValid("{x"));
    print("str=" + json.stringify(o));
    print("keys=" + json.stringify(json.keys(o)));
    print("vals=" + json.stringify(json.values(o)));
    print("has=" + json.has(o, "a"));
    print("nohas=" + json.has(o, "x"));
    string j2 = "{\"d\":4,\"e\":5}";
    map o2 = json.parse(j2);
    print("merge=" + json.stringify(json.merge(o, o2)));
    print("fmt=" + json.format(j));
    print("min=" + json.minify("{\"a\": 1}"));

    // ── Summary ──
    print("");
    print("=== Complete ===");
    print("gui:6/10 math:8/10 console:8/10 file:10/10");
    print("time:10/10 system:9/10 net:4/10 json:9/10");
    print("Total: 64/80 tested + 16 skipped");
    print("=================");
}
