function main() {
    printf("--- Sort Test ---");
    var arr = json.parse("[3, 1, 4, 1, 5, 9, 2, 6]");
    printf("Before sort:");
    print(arr);
    arr.sort();
    printf("After sort:");
    print(arr);
    printf("--- Sort Done ---");
}
