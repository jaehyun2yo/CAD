"""
도무송 자동화: cut_line_generator (씨아기칼)
인쇄지 사이즈만큼 가로 길이가 벗어날 수 있게 도면에 씨아기칼 생성

씨아기칼: 인쇄지 가로 길이 경계에 삽입하는 보조 절단선.
도면이 인쇄지보다 클 때 분할 위치를 표시하며, 브릿지가 포함됨.

인쇄지 사이즈 (mm):
  A4: 210 x 297
  B4: 257 x 364
  A3: 297 x 420
  B3: 364 x 515
  국전: 636 x 939
"""

# 인쇄지 가로 사이즈 (mm)
PRINT_WIDTHS = {
    'A4': 210,
    'B4': 257,
    'A3': 297,
    'B3': 364,
    'A2': 420,
    '국전': 636,
    '4x6': 152,
}


def generate_cut_lines(doc, print_size='A3', bridge_width=3.0, bridge_spacing=20.0):
    """인쇄지 사이즈 기준으로 씨아기칼 라인을 생성한다.

    도면의 가로 길이가 인쇄지 가로보다 클 경우,
    인쇄지 가로 간격마다 수직 씨아기칼을 삽입한다.

    Args:
        doc: Document object
        print_size: 인쇄지 사이즈 키 ('A3', 'B4' 등)
        bridge_width: 브릿지 폭 (mm)
        bridge_spacing: 브릿지 간격 (mm)

    Returns:
        dict: {
            'lines': [{x, y1, y2}, ...] — 씨아기칼 위치,
            'count': int,
            'print_width': float,
            'message': str
        }
    """
    width = PRINT_WIDTHS.get(print_size)
    if width is None:
        return {
            'lines': [],
            'count': 0,
            'error': f'알 수 없는 인쇄지 사이즈: {print_size}',
            'message': f'오류: 인쇄지 사이즈 "{print_size}"를 찾을 수 없습니다.'
        }

    bounds = doc.bounds()
    drawing_width = bounds.max.x - bounds.min.x

    if drawing_width <= width:
        return {
            'lines': [],
            'count': 0,
            'print_width': width,
            'message': f'도면 가로({drawing_width:.0f}mm)가 인쇄지({print_size} {width}mm) 이내 — 씨아기칼 불필요'
        }

    # 인쇄지 가로 간격마다 수직 씨아기칼 위치 계산
    cut_lines = []
    x = bounds.min.x + width
    while x < bounds.max.x:
        cut_lines.append({
            'x': x,
            'y1': bounds.min.y - 5.0,  # 도면 아래 5mm 여유
            'y2': bounds.max.y + 5.0,  # 도면 위 5mm 여유
        })
        x += width

    return {
        'lines': cut_lines,
        'count': len(cut_lines),
        'print_width': width,
        'bridge_width': bridge_width,
        'bridge_spacing': bridge_spacing,
        'message': f'씨아기칼 {len(cut_lines)}개 생성 (인쇄지: {print_size} {width}mm, 도면: {drawing_width:.0f}mm)'
    }
