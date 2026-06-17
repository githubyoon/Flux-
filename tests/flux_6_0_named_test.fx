function double(int x) {
    return(x * 2);
}
function greaterThan5(int x) {
    return(x > 5);
}
function add(int acc, int x) {
    return(acc + x);
}
function main() {
    printf("--- Flux 6.0 Method Call Test ---");
    string jsonStr = "[1, 2, 3, 4, 5]";
    var data = json.parse(jsonStr);
    printf("JSON parsed:");
    print(data);
    var doubled = data.map(double);
    printf("Map (x * 2):");
    print(doubled);
    var even = doubled.filter(greaterThan5);
    printf("Filter (x > 5):");
    print(even);
    var sum = even.reduce(0, add);
    printf("Reduce (Sum):");
    print(sum);
    var length = data.len();
    printf("Length:");
    print(length);
    data.append(100);
    printf("After append:");
    print(data);
    try {
        printf("Dangerous operation...");
        throw("custom error!");
    } catch (e) {
        printf("Exception caught:");
        print(e);
    }
    printf("--- Test Complete ---");
}
