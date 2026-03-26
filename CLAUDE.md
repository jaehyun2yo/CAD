# YJ-CAD — 도무송 목형 전용 2D/3D CAD 프로그램

## 답변 언어
- 모든 답변은 한글로 작성한다.
- 코드 주석, 커밋 메시지, 변수명은 영어.

## 기술 스택
- **언어:** C++17 (MSVC 14.29, VS 2019 Build Tools)
- **UI:** Qt6 6.5.3 (MSVC 2019 x64)
- **렌더링:** Custom OpenGL 3.3+ (정적 VBO + 부분 업데이트)
- **기하학:** Boost.Geometry + Clipper2
- **DXF:** dxflib
- **플러그인:** pybind11 (Python 3.13)
- **테스트:** Google Test (TDD)
- **빌드:** CMake 4.3 + vcpkg

## 빌드 명령
```bash
cmake --preset default
cmake --build build --config Debug
ctest --preset test
```

## 프로젝트 구조
```
src/
  core/
    document/   # Entity, Layer, Document, Command, Selection
    geometry/   # Boost.Geometry + Clipper2 래핑
    renderer/   # Custom OpenGL 렌더링 엔진
    fileio/     # dxflib DXF 파서, AI/EPS 임포트
    plugin/     # pybind11 Python 플러그인 시스템
  app/          # main.cpp, 앱 진입점
  ui/           # Qt6 UI 위젯 (패널, 대화상자)
tests/          # Google Test 테스트
plugins/
  domusong/     # 도무송 자동화 Python 애드온
resources/
  icons/        # 툴바 아이콘
  shaders/      # OpenGL 셰이더
```

## 코딩 컨벤션
- **네이밍:** camelCase (변수/함수), PascalCase (클래스), m_ 접두사 (멤버)
- **네임스페이스:** `yjcad`
- **헤더 가드:** `#pragma once`
- **함수 길이:** 최대 40줄 권장
- **에러 처리:** 예외 대신 반환값 + nullptr 체크
- **Undo/Redo:** 모든 편집 명령은 Command 패턴으로 구현

## M1 구현 순서 (12주)
1. Week 1-2: Document Model ✅
2. Week 3-4: File I/O (DXF 로드/저장)
3. Week 5-8: Renderer (Custom OpenGL)
4. Week 9-10: Geometry (Boost + Clipper2)
5. Week 11-12: Plugin System (pybind11)

## 테스트
- TDD: Geometry, Document 모듈
- 통합 테스트: File I/O (실제 DXF 파일)
- 정밀도: epsilon 1e-6 비교

## 핵심 도메인 (도무송)
- 오시선: 접선 (종이를 접는 눌림 선) — 기본 빨간색
- 칼선: 절단선 (종이를 자르는 선) — 기본 흰색
- 브릿지: 칼선의 끊김 구간 (목형 칼날 연결)
- 정타/후타: 칼 방향. 정타=뒤집기 필요
- 파일명 규칙: `MMDD-순번 업체명 도면명 절수.DXF`
