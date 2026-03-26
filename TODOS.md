# YJ-CAD 남은 작업 목록

> 마지막 업데이트: 2026-03-26
> M1 핵심 기능 완료. 아래는 남은 개선/확장 작업.

---

## 우선순위 P1 — 실사용을 위해 필요

### TODO-01: 호(Arc) 그리기 도구
- **What:** DrawArcTool 구현 (중심+반지름+시작/끝 각도 방식)
- **Why:** DXF 도면에 호가 많음. 편집 후 새로 그릴 수 있어야 함.
- **Effort:** S (2시간)
- **Files:** `src/ui/tools/draw_arc_tool.h/cpp`, ToolManager에 등록, main.cpp 메뉴 추가
- **Pattern:** DrawCircleTool과 유사. 3클릭: 중심→시작점→끝점.
- **Depends on:** 없음

### TODO-02: 미러 도구 (인터랙티브)
- **What:** MirrorTool 구현 — 선택 엔티티를 축 기준으로 뒤집기
- **Why:** 정타/후타 작업에 필수. 현재 DomusongPanel에서 하드코딩으로 처리 중.
- **Effort:** S (2시간)
- **Files:** `src/ui/tools/mirror_tool.h/cpp`, ToolManager 등록
- **Pattern:** 2클릭: 미러축의 시작점→끝점. Entity::mirror() 이미 구현됨.
- **Depends on:** 없음

### TODO-03: 스케일 도구
- **What:** ScaleTool 구현 — 선택 엔티티를 기준점 중심으로 확대/축소
- **Why:** 도면 크기 조정 필요. Entity::scale() 이미 구현됨.
- **Effort:** S (1시간)
- **Files:** `src/ui/tools/scale_tool.h/cpp`, ToolManager 등록
- **Pattern:** 2클릭: 기준점→스케일 팩터(거리 비율). MoveCommand 패턴 참고.
- **Depends on:** 없음

### TODO-04: 오프셋 도구 (인터랙티브)
- **What:** 선택된 폴리라인/선을 지정 거리만큼 오프셋
- **Why:** 도무송 외곽선 작업에 핵심. Clipper2 GeometryOps::offsetPolyline() 이미 구현됨.
- **Effort:** M (3시간)
- **Files:** `src/ui/tools/offset_tool.h/cpp`
- **Pattern:** 엔티티 선택 → 마우스로 오프셋 방향/거리 지정 → 새 엔티티 생성
- **Depends on:** 없음

### TODO-05: 트림/연장 도구
- **What:** TrimTool — 교차점에서 선분 자르기, ExtendTool — 교차점까지 연장
- **Why:** 도면 편집의 가장 기본 도구. Dream CAD에서 매일 사용.
- **Effort:** L (6시간)
- **Files:** `src/ui/tools/trim_tool.h/cpp`, `src/core/geometry/operations.h`에 교차점 계산 추가
- **Pattern:** 경계 엔티티 선택 → 자를 부분 클릭 → 교차점에서 분할/삭제
- **Depends on:** 선-선, 선-원 교차점 계산 함수 필요 (operations.h에 추가)

### TODO-06: 좌측 아이콘 툴바
- **What:** 왼쪽에 아이콘 기반 도구 모음 추가 (QToolBar)
- **Why:** 메뉴로만 도구 접근은 불편. 디자인 문서의 레이아웃에 포함됨.
- **Effort:** M (3시간)
- **Files:** `src/app/main.cpp`에 QToolBar 추가, `resources/icons/` 아이콘 필요
- **Pattern:** Qt6 QToolBar + QAction. 아이콘은 24x24 SVG 또는 PNG.
- **Note:** 아이콘은 LibreCAD 또는 Material Icons에서 가져오거나 AI로 생성 가능.
- **Depends on:** 없음

### TODO-07: 속성 패널 (Properties Tab)
- **What:** 오른쪽 패널에 "속성" 탭 추가 — 선택된 엔티티의 레이어/색상/좌표 편집
- **Why:** 엔티티 속성을 UI에서 직접 수정할 수 없음. 현재 코드로만 가능.
- **Effort:** M (4시간)
- **Files:** `src/ui/properties_panel.h/cpp`
- **Pattern:** QDockWidget + QTabWidget. 선택 변경 시 signaI로 패널 업데이트.
- **Depends on:** 없음

### TODO-08: 레이어 패널 (Layer Tab)
- **What:** 오른쪽 패널에 "레이어" 탭 추가 — 레이어 목록, 가시성 토글, 색상 변경
- **Why:** DXF 파일에 다수 레이어. 가시성 제어 필수.
- **Effort:** M (4시간)
- **Files:** `src/ui/layer_panel.h/cpp`
- **Pattern:** QListWidget + 체크박스(가시성) + 색상 버튼. LayerManager 연동.
- **Depends on:** 없음

---

## 우선순위 P2 — 품질 향상

### TODO-09: 한글 텍스트 렌더링 (FreeType)
- **What:** OpenGL에서 한글 텍스트를 실제로 렌더링 (현재 "T" 마커만 표시)
- **Why:** 도면 내 텍스트(업체명, 도면명) 표시 필요. info_text 단계 결과 확인 불가.
- **Effort:** L (8시간)
- **Files:** `src/core/renderer/text_renderer.h/cpp`, FreeType 라이브러리 추가
- **Pattern:** FreeType으로 글리프 → 텍스처 아틀라스 → OpenGL 쿼드 렌더링.
- **vcpkg:** `vcpkg install freetype:x64-windows`
- **Depends on:** 없음

### TODO-10: 비동기 DXF 로딩 (QThread)
- **What:** 대형 DXF 파일 로드 시 UI 멈추지 않도록 백그라운드 스레드 사용
- **Why:** 수만 개 엔티티 DXF는 로드에 수초 걸림. UI가 멈추면 앱이 죽은 것처럼 보임.
- **Effort:** M (4시간)
- **Files:** `src/core/fileio/dxf_reader.cpp` (QThread로 래핑), 프로그레스 시그널 추가
- **Pattern:** QThread + signal(progress, finished, error). main.cpp에서 프로그레스 바 연결.
- **Depends on:** 없음

### TODO-11: Light 테마
- **What:** 다크/라이트 테마 전환 기능
- **Why:** 디자인 리뷰에서 결정됨 (M1: Dark 우선, M2: Light 추가). 일부 사용자 선호.
- **Effort:** S (2시간)
- **Files:** `src/app/main.cpp` 스타일시트 분리, 보기 메뉴에 테마 전환 추가
- **Pattern:** 두 세트의 QSS 스타일시트. 캔버스 배경색도 #000000 ↔ #FFFFFF 전환.
- **Depends on:** 없음

### TODO-12: 앱 로그 시스템
- **What:** `%APPDATA%/YJ-CAD/logs/`에 앱 로그 저장. 크래시 시 마지막 로그 표시.
- **Why:** CEO 리뷰에서 발견된 누락 항목. 다수 작업자 사용 시 버그 추적 필요.
- **Effort:** S (2시간)
- **Files:** `src/core/logger.h/cpp`, main.cpp에서 초기화
- **Pattern:** qInstallMessageHandler()로 Qt 메시지 → 파일 리다이렉트.
- **Depends on:** 없음

---

## 우선순위 P3 — 확장

### TODO-13: AI/EPS 파일 임포트
- **What:** Adobe Illustrator (.ai) 및 EPS 파일을 DXF로 변환하여 임포트
- **Why:** 거래처가 AI/EPS 포맷으로 도면을 보내는 경우 있음.
- **Effort:** L (8시간)
- **Files:** `src/core/fileio/ai_eps_reader.h/cpp`
- **Pattern:** Ghostscript로 AI/EPS → SVG 변환 → SVG 파싱 → 내부 엔티티 생성
- **Risk:** AI 파일의 벡터 경로 추출 정확도 미검증. 지원 버전 제한 필요.
- **Depends on:** Ghostscript 설치

### TODO-14: 네스팅 프로그램 연동
- **What:** 가공 완료 DXF를 네스팅 프로그램의 감시 폴더에 자동 복사
- **Why:** 현재 수동으로 파일 이동. 워크플로우 자동화의 다음 단계.
- **Effort:** S (1시간)
- **Files:** auto_saver.py에 복사 경로 추가
- **Pattern:** 저장 후 shutil.copy로 네트워크 경로에 복사.
- **Depends on:** 네스팅 프로그램의 감시 폴더 경로 확인

### TODO-15: Phase 2 — 3D 뷰어
- **What:** 전개도 → 3D 접기 시각화
- **Why:** 디자인 문서 Phase 2. 도면이 접혔을 때 어떤 모양인지 미리보기.
- **Effort:** XL
- **Depends on:** M1 완전 안정화 후

### TODO-16: Phase 3 — 3D 모델링
- **What:** 3D 기하학 커널 (OpenCASCADE), 3D 편집 도구
- **Effort:** XL
- **Depends on:** Phase 2 완료

---

## 완료된 작업

- [x] Document Model (Entity 5종, Layer, Selection, Undo/Redo)
- [x] File I/O (DXF 로드/저장, atomic save, 파일명 규칙)
- [x] Renderer (Custom OpenGL 3.3, VBO, 그리드, 히트테스트)
- [x] Geometry (오프셋 Clipper2, 브릿지 삽입, 점빼기)
- [x] Plugin System (pybind11 전체 API)
- [x] Snap Engine (그리드/끝점/중점/중심)
- [x] 그리기 도구 (선, 원, 폴리라인)
- [x] 편집 도구 (이동, 복사, 회전, 삭제)
- [x] 고무줄 미리보기 + 스냅 인디케이터
- [x] 도무송 패널 (10단계 파이프라인 UI)
- [x] Python 플러그인 (cleanup, classifier, outline, info_text, auto_saver, pipeline, cut_line, orientation)
- [x] 다크 테마 UI
- [x] CMake + vcpkg 빌드 시스템
- [x] 74 tests (67 C++ + 7 Python)
