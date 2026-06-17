# 🌌 flux + (플럭스 플러스)

**flux +**는 C 언어의 직관적인 저수준 제어 감성과 Python/Go의 현대적인 편의 기능을 결합한 **C++20 기반의 강력한 인터프리터 언어**입니다.

## 🚀 주요 특징

- **타입 명시**: `int`, `float`, `string`, `bool`, `map` 타입을 명시하여 Zero Value 자동 초기화를 지원합니다. 타입 검사는 런타임에 수행됩니다.
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

### 1-1. 맵(Map) 타입
```flux
map config = json.parse("{\"key\": \"value\"}");
print(json.stringify(config));
```

### 2. 조건문 및 반복문

#### 2-1. if / else if / else
```flux
if (score >= 90) {
    print("Grade: A");
} else if (score >= 80) {
    print("Grade: B");
}
```

> 조건식에서 `and`, `or`, `not`을 `&&`, `||`, `!` 대신 사용할 수 있습니다:
> ```flux
> if (x > 0 and x < 10) { }
> if (not done) { }
> ```

#### 2-2. for 반복문
```flux
for (int i = 0; i < 5; i++) {
    printf("현재 카운트: {i}");
}
```

> *(참고: `for` 반복문은 현재 트리-워크 인터프리터에서만 동작합니다. 바이트코드 VM에서는 `while`을 대신 사용하세요.)*

#### 2-3. while 반복문
```flux
while (condition) {
    print("Looping...");
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

### 5. 스레드 생성 (spawn)
```flux
function worker(int id) {
    printf("Worker {id} started");
}

spawn(worker, 1);
spawn(worker, 2);
```
`spawn`은 새로운 OS 스레드에서 함수를 실행합니다. 모든 스레드는 전역 변수를 공유합니다.

### 6. 배열 메서드
```flux
int[] arr = [1, 2, 3, 4, 5];

var doubled = arr.map(function(x) { return x * 2; });     // [2, 4, 6, 8, 10]
var even = arr.filter(function(x) { return x > 2; });      // [3, 4, 5]
var sum = arr.reduce(0, function(acc, x) { return acc + x; }); // 15
arr.sort();                                                  // 정렬
int len = arr.len();                                         // 5
arr.append(6);                                               // [1,2,3,4,5,6]
```

### 7. 모듈 임포트
```flux
import "math";                 // 표준 모듈 임포트
import greet from "lib.fx";   // 외부 파일에서 함수 임포트
```

## 📦 표준 모듈 목록

| 모듈 | 함수 목록 (총 10개) |
| :--- | :--- |
| **`gui`** | `msgbox`, `window`, `button`, `loop`, `label`, `close`, `setTitle`, `getX`, `getY`, `show` |
| **`net`** | `get`, `post`, `put`, `del`, `encode`, `decode`, `download`, `status`, `headers`, `ip` |
| **`system`** | `exit`, `run`, `os`, `env`, `cpu`, `pid`, `user`, `cwd`, `temp`, `host` |
| **`console`** | `clear`, `title`, `color`, `input`, `width`, `height`, `cursor`, `readkey`, `beep`, `reset` |
| **`file`** | `read`, `write`, `append`, `exists`, `remove`, `size`, `is_dir`, `mkdir`, `copy`, `rename` |
| **`math`** | `abs`, `round`, `sin`, `cos`, `tan`, `sqrt`, `pow`, `log`, `pi`, `e` |
| **`time`** | `sleep`, `now`, `ticks`, `year`, `month`, `date`, `hour`, `minute`, `second`, `format` |
| **`json`** | `parse`, `stringify`, `isValid`, `format`, `minify`, `keys`, `values`, `has`, `merge`, `type` |

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
