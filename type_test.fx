function main() {
    print("--- [v6.1] 내장 메서드 테스트 ---");
    
    // 1. String 메서드
    string s = "Flux Plus";
    print(s.len());
    print(s.upper());
    
    // 2. Array 메서드
    int nums[];
    nums.append(10);
    nums.append(20);
    print(nums.len());
    
    // 3. 숫자 메서드
    float val = -3.7;
    print(val.abs());
    print(val.round());
}
