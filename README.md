# 🌌 flux + (플럭스 플러스)

**flux +**는 C 언어의 직관적인 저수준 제어 감성과 Python/Go의 현대적인 편의 기능을 결합한 **C++20 기반의 강력한 인터프리터 언어**입니다.

## 🚀 주요 특징

- **정적 타입 지향**: `int`, `float`, `string`, `bool` 타입을 명시하여 안정적인 코딩이 가능합니다.
- **Zero Value 정책**: 선언 후 초기화하지 않은 변수는 타입별 기본값(`0`, `false`, `""`)으로 자동 초기화됩니다.
- **객체 지향 프로그래밍 (OOP)**: `struct`와 `class`를 지원하며, `this` 키워드를 통한 메서드 컨텍스트 제어가 가능합니다.
- **견고한 예외 처리**: `try-catch-throw` 구문을 통해 런타임 오류를 우아하게 관리합니다.
- **강력한 출력 시스템**: 파이썬 f-string 스타일의 `{}` 인터폴레이션을 지원하는 `printf()`를 탑재했습니다.
- **네이티브 GUI 모듈**: Windows API와 완벽하게 연동되어 별도 라이브러리 없이 창(Window)을 띄울 수 있습니다.
- **UTF-8 한글 지원**: Windows 터미널 환경에서도 한글이 깨짐 없이 완벽하게 출력됩니다.

## 🛠️ 문법 가이드

### 1. 진입점 및 변수 선언
```flux
function main() {
    int score;        // 0으로 초기화
    string name;      // ""으로 초기화
    name = "Fluxer";
    score = 100;
}
```

### 2. 조건문 및 반복문
```flux
if (score >= 90) {
    print("Grade: A");
} else if (score >= 80) {
    print("Grade: B");
}

for (int i = 0; i < 5; i++) {
    printf("현재 카운트: {i}");
}
```

### 3. 클래스 및 메서드 (v4.1)
```flux
class Player {
    int hp;
    function heal(int amount) {
        this.hp = this.hp + amount;
        printf("치유 완료! 현재 체력: {this.hp}");
    }
}
```

### 4. 예외 처리
```flux
try {
    throw("Fatal Error!");
} catch (e) {
    printf("에러 발생: {e}");
}
```

## 📦 표준 모듈 목록

| 모듈 | 주요 기능 |
| :--- | :--- |
| **`gui`** | 메시지 박스 팝업, 윈도우 창 생성, **버튼 컨트롤 추가**, 이벤트 루프 |
| **`net`** | **HTTP GET/POST 요청**, 웹 데이터 수집 |
| **`system`** | **OS 쉘 실행**, 플랫폼 확인, 환경 변수 읽기, 종료 |
| **`console`** | **사용자 입력(input)**, 화면 초기화, 텍스트 색상 변경, 커서 조작, 창 제목 설정 |
| **`file`** | 파일 읽기/쓰기/추가(append), **디렉토리 생성**, 존재 확인, 크기 및 타입 확인 |
| **`math`** | sqrt, pow, abs, sin, cos, tan, log, pi, e |
| **`time`** | **정밀 틱(ticks) 측정**, 밀리초/초 단위 지연, 현재 시간 포맷팅 |
| **`random`** | 난수 생성, 시드 설정 |

## 🏗️ 빌드 방법 (Windows MSVC 기준)

제공된 `build.bat`을 실행하거나 아래 명령어를 사용하세요:

```powershell
cl /std:c++20 /utf-8 /EHsc /Iinclude src/*.cpp /Feflux_interpreter.exe
```

## 🧪 실행 방법

```powershell
.\flux_interpreter.exe your_script.fx
```

---
**flux +**는 계속해서 진화하고 있습니다. 현재 강력한 **스택 기반 바이트코드 VM**을 통해 고성능 실행을 지원합니다.
