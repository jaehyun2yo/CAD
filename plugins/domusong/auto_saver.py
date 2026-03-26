"""
도무송 자동화: auto_saver
파일명 규칙에 맞게 자동 저장

파일명 규칙: MMDD-순번 업체명 도면명 절수.DXF
예: 0326-1 원컴퍼니 퀘이크업미니가차 2절.DXF
"""

import os
from datetime import datetime


def generate_save_path(base_dir, seq, company, drawing_name, jeolsu,
                       date=None):
    """파일명 규칙에 맞는 저장 경로를 생성한다.

    Args:
        base_dir: 저장 폴더 (예: '\\\\192.168.0.6\\home\\데이터\\유진MAIN\\dxf')
        seq: 순번 (1, 2, 3, ...)
        company: 업체명
        drawing_name: 도면명
        jeolsu: 절수 ('2절', '4절' 등)
        date: 날짜 (기본: 오늘)

    Returns:
        str: 전체 파일 경로
    """
    if date is None:
        now = datetime.now()
        date_str = f'{now.month:02d}{now.day:02d}'
    else:
        date_str = date

    filename = f'{date_str}-{seq} {company} {drawing_name} {jeolsu}.DXF'
    return os.path.join(base_dir, filename)


def get_next_seq(base_dir, date=None):
    """오늘 날짜의 다음 순번을 계산한다.

    Args:
        base_dir: DXF 저장 폴더
        date: 날짜 문자열 'MMDD' (기본: 오늘)

    Returns:
        int: 다음 순번
    """
    if date is None:
        now = datetime.now()
        date_str = f'{now.month:02d}{now.day:02d}'
    else:
        date_str = date

    max_seq = 0
    prefix = f'{date_str}-'

    try:
        for fname in os.listdir(base_dir):
            if fname.upper().startswith(prefix):
                # Extract sequence number: MMDD-N ...
                rest = fname[len(prefix):]
                space_idx = rest.find(' ')
                if space_idx > 0:
                    try:
                        seq = int(rest[:space_idx])
                        max_seq = max(max_seq, seq)
                    except ValueError:
                        pass
    except (OSError, FileNotFoundError):
        pass

    return max_seq + 1
