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


# ── 네스팅 프로그램 연동 ──

# 네스팅 프로그램 감시 폴더 (기본값)
NESTING_WATCH_DIR = r'\\192.168.0.6\home\데이터\유진MAIN\nesting_input'


def copy_to_nesting(dxf_path, nesting_dir=None):
    """가공 완료 DXF를 네스팅 프로그램의 감시 폴더에 복사한다.

    Args:
        dxf_path: 저장된 DXF 파일 경로
        nesting_dir: 네스팅 감시 폴더 (기본: NESTING_WATCH_DIR)

    Returns:
        str: 복사된 파일 경로, 실패 시 None
    """
    import shutil

    if nesting_dir is None:
        nesting_dir = NESTING_WATCH_DIR

    if not os.path.exists(dxf_path):
        return None

    # Create nesting dir if it doesn't exist
    try:
        os.makedirs(nesting_dir, exist_ok=True)
    except OSError:
        return None

    dest = os.path.join(nesting_dir, os.path.basename(dxf_path))
    try:
        shutil.copy2(dxf_path, dest)
        return dest
    except (OSError, shutil.Error):
        return None


def save_and_send_to_nesting(base_dir, seq, company, drawing_name, jeolsu,
                              nesting_dir=None, date=None):
    """저장 경로를 생성하고, 네스팅 폴더에도 자동 복사한다.

    Args:
        base_dir: DXF 저장 폴더
        seq: 순번
        company: 업체명
        drawing_name: 도면명
        jeolsu: 절수
        nesting_dir: 네스팅 감시 폴더 (기본: NESTING_WATCH_DIR)
        date: 날짜 (기본: 오늘)

    Returns:
        tuple: (save_path, nesting_path) — nesting_path는 실패 시 None
    """
    save_path = generate_save_path(base_dir, seq, company, drawing_name,
                                    jeolsu, date)
    nesting_path = copy_to_nesting(save_path, nesting_dir)
    return save_path, nesting_path
