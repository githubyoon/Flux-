> ?? **빠른 실행이 필요하신가요?**
> [`GIT_FLOW_CHEATSHEET.md`](GIT_FLOW_CHEATSHEET.md) 에서 상황별 명령어를 바로 확인하세요.

---
# Git Flow 정책 ? flux+

> 소규모 1인 개발에 최적화된 실용적인 Git Flow 정책

---

## 1. 브랜치 구조

```
main (배포/운영)
  └── develop (통합 개발)
        ├── feature/* (기능 개발)
        └── hotfix/* (긴급 수정: main에서 분기)
```

| 브랜치 | 설명 |
|--------|------|
| **main** | 배포 가능한 안정 버전만 존재. 직접 커밋 금지. |
| **develop** | 기능 개발 통합 브랜치. 기본 작업 브랜치. |
| **feature/*** | 기능 단위 개발 브랜치. |
| **hotfix/*** | 배포 버전 긴급 수정 브랜치. `main`에서 분기. |

---

## 2. 브랜치 네이밍 컨벤션

| 브랜치 | 패턴 | 예시 |
|--------|------|------|
| 통합 | `main`, `develop` | 고정 |
| 기능 | `feature/<kebab-case>` | `feature/bytecode-vm` |
| 수정 | `hotfix/<version-or-desc>` | `hotfix/1.2.1` |
| 릴리스 | `release/<version>` | `release/1.3.0` |

- 영문 소문자 + 케밥 케이스(kebab-case) 원칙
- 숫자 사용 가능

---

## 3. 커밋 메시지 컨벤션 (Conventional Commits 기반)

```
<type>: <간결한 설명>

<선택적 상세 설명>
```

### 타입

| 타입 | 의미 |
|------|------|
| `feat` | 새로운 기능 추가 |
| `fix` | 버그 수정 |
| `refactor` | 코드 리팩토링 (기능 변경 없음) |
| `docs` | 문서 작업 |
| `test` | 테스트 코드 추가/수정 |
| `chore` | 빌드 설정, 기타 잡일 |
| `perf` | 성능 개선 |
| `style` | 코드 포맷팅 (로직 변경 없음) |

### 예시

```
feat: 바이트코드 VM 초안 구현

- 3-주소 코드 기반 명령어 집합 설계
- 기본 연산 (ADD, SUB, MUL, DIV) 실행 추가
```

---

## 4. 브랜치 운영 규칙

| 규칙 | 설명 |
|------|------|
| main 직접 푸시 금지 | 모든 변경은 feature → develop → main 흐름으로만 |
| develop이 기본 브랜치 | 기본 작업은 develop에서, 필요시 feature 브랜치 분기 |
| feature 완료 후 병합 | feature 브랜치 작업 완료 → develop으로 병합 후 브랜치 삭제 |
| hotfix는 main에서 분기 | 긴급 수정은 main → hotfix → main & develop |
| 태그로 버전 표시 | main 병합 시 `v1.2.3` 형식의 annotated tag 생성 |

---

## 5. 1인 개발자를 위한 실용적 조정

- **feature 브랜치는 선택 사항**: 아주 작은 변경은 develop에 직접 커밋 가능
- **squash merge 권장**: feature 브랜치 작업 내역을 하나의 커밋으로 압축
- **hotfix**: main에서 분기하여 수정 후 main과 develop 양쪽에 병합
- **릴리스 주기**: 개발 완료 시점에 `release/` 브랜치 생성 (생략 가능)

---

## 6. 워크플로우 예시

```bash
# 1. develop에서 새 기능 브랜치 생성
git checkout develop
git checkout -b feature/sample-feature

# 2. 작업 후 커밋 (변경된 소스만)
git add -u
git commit -m "feat: 샘플 기능 구현"

# 3. develop에 병합
git checkout develop
git merge --squash feature/sample-feature
git commit -m "feat: 샘플 기능 구현"

# 4. feature 브랜치 삭제
git branch -d feature/sample-feature

# 5. develop을 원격에 푸시
git push origin develop

# 6. 배포 준비 완료 시 main으로 병합
git checkout main
git merge develop
git tag -a v1.0.0 -m "Release v1.0.0"
git push origin main --tags

# 7. develop으로 돌아가서 계속 작업
git checkout develop
```



