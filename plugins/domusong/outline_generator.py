"""
도무송 자동화: outline_generator
절수/동판/자동판에 맞는 외곽선 자동 생성

외곽선 규격 (기본값 — 도메인 규칙 수집 후 업데이트):
  - 절수: 합판 크기에 따른 외곽선 (2절, 4절 등)
  - 동판: 동판 가공용 외곽선 규격
  - 자동판: 자동 타발기용 외곽선 규격

외곽선은 도면의 바운딩 박스를 기준으로 여유분을 추가하여 생성.
"""

# 합판 사이즈 (mm) — 절수별 표준 규격
PLYWOOD_SIZES = {
    '2절': (600, 900),
    '4절': (450, 600),
    '8절': (300, 450),
    '국2절': (545, 788),
    '국4절': (394, 545),
    '국8절': (272, 394),
}

# 외곽선 여유분 (mm)
OUTLINE_MARGINS = {
    '동판': {'top': 15, 'bottom': 15, 'left': 15, 'right': 15},
    '자동판': {'top': 20, 'bottom': 25, 'left': 20, 'right': 20},
    '일반': {'top': 10, 'bottom': 10, 'left': 10, 'right': 10},
}


def generate_outline(doc, outline_type='일반', jeolsu=None):
    """도면 바운딩 박스 기준으로 외곽선을 생성한다.

    Args:
        doc: Document object
        outline_type: '동판', '자동판', '일반' 중 선택
        jeolsu: 절수 ('2절', '4절' 등) — None이면 바운딩 박스 기반

    Returns:
        dict: {
            'outline_id': EntityId of created outline,
            'width': float, 'height': float,
            'message': str
        }
    """
    bounds = doc.bounds()
    margins = OUTLINE_MARGINS.get(outline_type, OUTLINE_MARGINS['일반'])

    if jeolsu and jeolsu in PLYWOOD_SIZES:
        # 절수 기반: 합판 크기 사용
        pw, ph = PLYWOOD_SIZES[jeolsu]
        # 도면을 합판 중앙에 배치
        cx = (bounds.min.x + bounds.max.x) / 2.0
        cy = (bounds.min.y + bounds.max.y) / 2.0
        x1 = cx - pw / 2.0
        y1 = cy - ph / 2.0
        x2 = cx + pw / 2.0
        y2 = cy + ph / 2.0
    else:
        # 바운딩 박스 + 여유분
        x1 = bounds.min.x - margins['left']
        y1 = bounds.min.y - margins['bottom']
        x2 = bounds.max.x + margins['right']
        y2 = bounds.max.y + margins['top']

    width = x2 - x1
    height = y2 - y1

    # Create outline as a closed polyline
    # Note: actual entity creation should be done through the C++ API
    # This returns the parameters for the C++ side to create
    return {
        'vertices': [
            {'x': x1, 'y': y1},
            {'x': x2, 'y': y1},
            {'x': x2, 'y': y2},
            {'x': x1, 'y': y2},
        ],
        'closed': True,
        'classification': 5,  # LineClassification::Outline
        'layer': 'outline',
        'width': width,
        'height': height,
        'outline_type': outline_type,
        'jeolsu': jeolsu or 'auto',
        'message': f'외곽선 생성: {outline_type} {width:.0f}x{height:.0f}mm'
    }
