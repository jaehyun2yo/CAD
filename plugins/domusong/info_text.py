"""
도무송 자동화: info_text
합판 왼쪽 상단에 정보 텍스트 자동 작성

포맷: 날짜 업체명 도면명 인쇄지사이즈
위치: 합판 바운딩 박스의 좌상단
"""

from datetime import datetime


def generate_info_text(doc, company, drawing_name, print_size='',
                       date=None, position=None):
    """합판 좌상단에 정보 텍스트를 생성한다.

    Args:
        doc: Document object
        company: 업체명 (예: '원컴퍼니')
        drawing_name: 도면명 (예: '퀘이크업미니가차')
        print_size: 인쇄지 사이즈 (예: 'A4', 'B4')
        date: 날짜 문자열 (기본: 오늘)
        position: 텍스트 위치 (기본: 도면 좌상단)

    Returns:
        dict: text entity parameters for C++ side
    """
    if date is None:
        date = datetime.now().strftime('%m/%d')

    bounds = doc.bounds()

    if position is None:
        # 합판 좌상단 (바운딩 박스 위쪽에 배치)
        x = bounds.min.x
        y = bounds.max.y + 5.0  # 5mm above drawing
    else:
        x, y = position['x'], position['y']

    parts = [date, company, drawing_name]
    if print_size:
        parts.append(print_size)
    text_content = ' '.join(parts)

    return {
        'text': text_content,
        'position': {'x': x, 'y': y},
        'height': 5.0,  # 5mm text height
        'layer': '0',
        'message': f'정보 텍스트: {text_content}'
    }
