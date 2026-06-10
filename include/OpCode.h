#ifndef FLUX_OPCODE_H
#define FLUX_OPCODE_H

#include <cstdint>

namespace Flux {

enum OpCode : uint8_t {
    OP_CONSTANT,      // 상수 풀에서 값을 가져와 스택에 푸시
    OP_NULL,
    OP_TRUE,
    OP_FALSE,
    
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,

    OP_PRINT,
    OP_POP,           // 스택 상단 값 버리기

    OP_DEFINE_GLOBAL, // 전역 변수 정의
    OP_GET_GLOBAL,    // 전역 변수 읽기
    OP_SET_GLOBAL,    // 전역 변수 설정
    
    OP_GET_LOCAL,     // 지역 변수 읽기
    OP_SET_LOCAL,     // 지역 변수 설정

    OP_JUMP,          // 무조건 점프
    OP_JUMP_IF_FALSE, // 조건부 점프
    OP_LOOP,          // 뒤로 점프 (루프)

    OP_CALL,          // 함수 호출
    OP_RETURN,        // 함수 반환

    OP_NEW,           // 객체 인스턴스 생성
    OP_GET_PROPERTY,  // 객체 속성 읽기
    OP_SET_PROPERTY,  // 객체 속성 설정

    OP_NEW_MAP,       // Map 인스턴스 생성
    OP_GET_MAP,       // Map 요소 읽기
    OP_SET_MAP,       // Map 요소 설정

    OP_DEFINE_STRUCT, // 구조체 정의 등록
    OP_DEFINE_CLASS,  // 클래스 정의 등록
};

} // namespace Flux

#endif
