"""
도무송 자동화: orientation_handler
정타/후타 판단 → 텍스트 작성 + 뒤집기

- 정타: 칼이 위를 향함. 합판에 텍스트를 뒤집어서(미러) 작성 필요.
- 후타: 칼이 아래를 향함. 합판 텍스트를 그대로 사용.

이 단계는 반자동 — 사용자가 정타/후타를 선택하면 자동으로 처리.
"""


def apply_orientation(doc, orientation='정타'):
    """정타/후타에 따라 도면을 처리한다.

    Args:
        doc: Document object
        orientation: '정타' 또는 '후타'

    Returns:
        dict: {
            'orientation': str,
            'mirrored': bool,
            'text_added': bool,
            'message': str
        }
    """
    bounds = doc.bounds()
    cx = (bounds.min.x + bounds.max.x) / 2.0

    if orientation == '정타':
        # 정타: 도면을 X축 기준으로 미러 (좌우 뒤집기)
        # 실제 미러는 C++ API를 통해 수행해야 함
        return {
            'orientation': '정타',
            'mirror_axis': 'x',
            'mirror_center_x': cx,
            'mirrored': True,
            'text_direction': 'mirrored',
            'text_content': f'정타 (뒤집음)',
            'text_position': {
                'x': bounds.min.x,
                'y': bounds.max.y + 10.0  # 도면 위 10mm
            },
            'message': '정타 처리: 도면 뒤집기(미러) + 텍스트 작성'
        }
    else:
        # 후타: 그대로 사용
        return {
            'orientation': '후타',
            'mirrored': False,
            'text_direction': 'normal',
            'text_content': f'후타',
            'text_position': {
                'x': bounds.min.x,
                'y': bounds.max.y + 10.0
            },
            'message': '후타 처리: 도면 그대로 + 텍스트 작성'
        }
