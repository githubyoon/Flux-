function test() {
    printf("1. 함수 시작");
    throw("에러 발생!");
    printf("2. 여기는 실행되면 안 됨");
}

function main() {
    try {
        printf("3. try 블록 시작");
        test();
        printf("4. 여기도 실행되면 안 됨");
    } catch (e) {
        printf("5. catch 블록 실행됨. 메시지: {e}");
    }
    printf("6. try-catch 이후 정상 실행");
}
