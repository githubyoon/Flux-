import gui;
import system; // 필수! 종료 기능을 위해 추가

// 버튼 클릭 시 실행될 콜백 함수
function on_click() {
    gui.msgbox("알림", "버튼이 클릭되었습니다! 이벤트 시스템이 작동 중입니다.");
}

function on_exit() {
    gui.msgbox("종료", "프로그램을 종료합니다.");
    system.exit(0); // 이제 정상 작동합니다.
}

function main() {
    // 1. 메인 윈도우 생성
    gui.window("Flux + Interactive App", 400, 300);

    // 2. 라벨 추가
    gui.label("아래 버튼을 눌러보세요:", 20, 20, 200, 25);

    // 3. 버튼 추가 (텍스트, x, y, w, h, 콜백함수이름)
    gui.button("클릭 미!", 20, 60, 100, 40, "on_click");
    
    gui.button("프로그램 종료", 20, 120, 100, 40, "on_exit");

    print("GUI 이벤트 루프 시작...");
    
    // 4. 이벤트 루프 실행
    gui.loop();
}
