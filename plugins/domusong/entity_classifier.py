"""
도무송 자동화: entity_classifier
오시/칼선 엔티티 자동 분류 (반자동 — AI 제안 + 사용자 확인)

분류 규칙 (기본값 — 도메인 규칙 수집 후 업데이트 필요):
  - 레이어명에 'oshi', '오시', 'crease', 'fold' → 오시
  - 레이어명에 'cut', '칼', 'knife', 'die' → 칼
  - 색상 1 (빨강) → 오시
  - 색상 7 (흰색) 또는 기본 → 칼
  - 나머지 → Unknown (사용자 확인 필요)
"""


def classify(doc):
    """모든 엔티티를 자동 분류하고 결과를 반환한다.

    Returns:
        dict: {
            'oshi': [entity_ids],
            'knife': [entity_ids],
            'unknown': [entity_ids],
            'total': int,
            'auto_classified': int
        }
    """
    oshi_keywords = ['oshi', '오시', 'crease', 'fold', '접']
    knife_keywords = ['cut', '칼', 'knife', 'die', '절단']

    result = {'oshi': [], 'knife': [], 'unknown': [], 'total': 0, 'auto_classified': 0}

    for entity_id in doc.entity_ids():
        entity = doc.entity(entity_id)
        if entity is None:
            continue

        result['total'] += 1
        layer_name = entity.layer_name.lower()
        color = entity.color
        classified = False

        # Rule 1: Layer name keywords
        for kw in oshi_keywords:
            if kw in layer_name:
                entity.classification = 1  # Oshi
                result['oshi'].append(entity_id)
                classified = True
                break

        if not classified:
            for kw in knife_keywords:
                if kw in layer_name:
                    entity.classification = 2  # Knife
                    result['knife'].append(entity_id)
                    classified = True
                    break

        # Rule 2: Color-based (fallback)
        if not classified:
            if color == 1:  # Red → Oshi
                entity.classification = 1
                result['oshi'].append(entity_id)
                classified = True
            elif color == 7 or color == 0:  # White/default → Knife
                entity.classification = 2
                result['knife'].append(entity_id)
                classified = True

        if not classified:
            result['unknown'].append(entity_id)
        else:
            result['auto_classified'] += 1

    return result
