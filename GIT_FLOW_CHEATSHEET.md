# Git Flow 치트시트 ? flux+

> ?? **Git Flow 정책 문서:** [GIT_FLOW.md](GIT_FLOW.md)

---

## 1. 한눈에 보는 Git Flow 구조도

```
main ──────●────────────●───────── (배포)
             \          /
develop ─────●────●────●────────── (통합)
               \  /  \
feature/*       ●    ●──────────── (기능 개발)
hotfix/*                  ●─●──   (긴급 수정, main에서 분기)
```

---

## 2. 지금 당장 개발 시작하기 (일일 워크플로우)

```bash
# 1단계: develop 브랜치로 이동 + 최신 동기화
git checkout develop
git pull origin develop

# 2단계: 새 기능 브랜치 만들기
git checkout -b feature/내기능

# 3단계: 작업 후 커밋
git add -u
git commit -m "feat: 내 기능 구현"

# 4단계: develop에 병합하고 푸시
git checkout develop
git merge --squash feature/내기능
git commit -m "feat: 내 기능 구현"
git branch -d feature/내기능
git push origin develop
```

---

## 3. 상황별 명령어 모음 (치트시트)

| 하고 싶은 일 | 명령어 |
|-------------|--------|
| develop에서 새 기능 개발 시작 | `git checkout develop; git pull; git checkout -b feature/기능명` |
| 기능 개발 완료 → develop 병합 | `git checkout develop; git merge --squash feature/기능명; git commit -m "feat: ..."; git branch -d feature/기능명; git push` |
| develop → main 배포 | `git checkout main; git merge develop; git tag -a v1.0.0 -m "Release v1.0.0"; git push origin main --tags; git checkout develop` |
| 긴급 hotfix 시작 (main 기준) | `git checkout main; git checkout -b hotfix/1.0.1` |
| hotfix → main + develop 병합 | `git checkout main; git merge hotfix/1.0.1; git tag -a v1.0.1 -m "..."; git checkout develop; git merge hotfix/1.0.1; git branch -d hotfix/1.0.1; git push --all --tags` |
| 원격 최신 상태로 동기화 | `git fetch --prune; git pull` |
| 현재 브랜치/상태 확인 | `git status; git branch -a; git log --oneline --graph -10` |

---

## 4. flux+ 실제 예시

```bash
# 예: 새 기능 "배열 내장 메서드" 개발
git checkout develop
git checkout -b feature/array-methods

# 작업 완료 후
git add -u
git commit -m "feat: 배열 내장 메서드(len, append, pop) 추가"

git checkout develop
git merge --squash feature/array-methods
git commit -m "feat: 배열 내장 메서드(len, append, pop) 추가"
git branch -d feature/array-methods
git push origin develop
```

---

## 5. 병합 충돌 해결 가이드

```bash
# 충돌 발생 시 메시지 예시:
# Auto-merging src/Compiler.cpp
# CONFLICT (content): Merge conflict in src/Compiler.cpp

# 1. 충돌난 파일 열기
# 2. <<<<<<< HEAD ... ======= ... >>>>>>> feature/xxx 부분 수정
# 3. 수정 완료 후:
git add src/Compiler.cpp
git commit -m "fix: 병합 충돌 해결"
```

---

## 6. 되돌리기 (언제나 구할 수 있는 방법)

```bash
# 커밋을 잘못했어요 (직전 커밋 수정)
git commit --amend -m "수정된 메시지"

# 스테이징을 취소하고 싶어요
git reset HEAD 파일명

# 최근 커밋을 취소하고 싶어요 (변경사항은 유지)
git reset --soft HEAD~1

# 작업 중인 변경사항을 모두 취소 (되돌릴 수 없음 주의)
git restore .
```

