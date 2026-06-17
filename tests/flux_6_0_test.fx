function main() {
    printf("--- Flux 6.0 실용성 테스트 ---");

    // 1. JSON & Array 메서드 테스트
    string jsonStr = "[1, 2, 3, 4, 5]";
    var data = json.parse(jsonStr);
    printf("JSON 파싱 결과: {data}");

    var doubled = data.map(function(x) { return x * 2; });
    printf("Map (x * 2): {doubled}");

    var even = doubled.filter(function(x) { return x > 5; });
    printf("Filter (x > 5): {even}");

    var sum = even.reduce(0, function(acc, x) { return acc + x; });
    printf("Reduce (Sum): {sum}");

    // 2. 예외 처리 테스트
    try {
        printf("위험한 작업 시작...");
        throw("커스텀 에러!");
    } catch (e) {
        printf("예외 포착: {e}");
    }

    printf("--- 테스트 완료 ---");
}
