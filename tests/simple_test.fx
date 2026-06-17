function main() {
    map jobj = json.parse("{\"a\":1}");
    string jt1 = json.type(42);
    print("type(42) = " + jt1);
    string jt2 = json.type("hello");
    print("type(hello) = " + jt2);
    string jt3 = json.type(true);
    print("type(true) = " + jt3);
    string jt4 = json.type(jobj);
    print("type(obj) = " + jt4);
    print("done");
}
