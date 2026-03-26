# YJ-CAD 세션 이어하기 프롬프트

아래 프롬프트를 새 Claude Code 세션에 붙여넣으세요.

---

## 프롬프트

```
YJ-CAD 프로젝트를 이어서 작업해줘.

## 프로젝트 정보
- GitHub: https://github.com/jaehyun2yo/CAD
- 위치: C:\Users\jaehy\OneDrive\Desktop\dev\projects\yjlaser\AT-test
- 기술: C++17, Qt6 6.5.3, Custom OpenGL, Boost.Geometry, Clipper2, libdxfrw, pybind11, GTest
- 빌드: cmake --preset default && cmake --build build --config Debug
- 테스트: ctest --test-dir build --build-config Debug (67 C++) + python tests/plugins/test_pipeline.py (7 Python)

## 현재 상태
M1+M2 핵심 기능 모두 완료. 5 commits, 74 tests 통과, ~6,500줄.
구현 완료: Document Model, DXF I/O, OpenGL Renderer, Geometry, Plugin System, Snap,
도무송 10단계 파이프라인, 그리기/편집 도구 (선/원/폴리/이동/복사/회전/삭제)

## 남은 작업
TODOS.md 참고. 우선순위별:

P1 (실사용 필수):
- TODO-01: 호 그리기 도구 (S)
- TODO-02: 미러 도구 (S)
- TODO-03: 스케일 도구 (S)
- TODO-04: 오프셋 도구 인터랙티브 (M)
- TODO-05: 트림/연장 도구 (L)
- TODO-06: 좌측 아이콘 툴바 (M)
- TODO-07: 속성 패널 (M)
- TODO-08: 레이어 패널 (M)

P2 (품질):
- TODO-09: FreeType 한글 텍스트 렌더링 (L)
- TODO-10: 비동기 DXF 로딩 (M)
- TODO-11: Light 테마 (S)
- TODO-12: 앱 로그 (S)

## 작업 방법
1. TODOS.md에서 작업할 TODO 선택
2. 해당 파일 생성/수정
3. CMakeLists.txt에 새 파일 추가
4. 빌드 + 테스트 확인
5. git commit + push

## 규칙
- CLAUDE.md 참고 (코딩 컨벤션, 네이밍)
- TDD: 새 기능은 테스트 먼저
- 함수 ≤ 40줄
- namespace yjcad, camelCase 변수, PascalCase 클래스, m_ 멤버
- 에러는 예외 대신 반환값 + nullptr 체크

P1 작업을 순서대로 진행해줘. TODO-01부터 시작.
```

---

## 빠른 시작 (1줄 버전)

```
TODOS.md의 P1 작업을 TODO-01부터 순서대로 구현해줘. 빌드+테스트 확인 후 커밋.
```
