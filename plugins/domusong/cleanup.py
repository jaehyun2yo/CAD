"""
도무송 자동화 플러그인: cleanup
불필요한 엔티티 자동 제거

사용법:
  import yjcad_core as cad
  from plugins.domusong.cleanup import run
  run(doc)
"""

def run(doc):
    """불필요한 엔티티 제거 — 0길이 선분, 보이지 않는 레이어의 엔티티 등"""
    removed = 0
    to_remove = []

    for entity_id in doc.entity_ids():
        entity = doc.entity(entity_id)
        if entity is None:
            continue

        # Remove zero-length lines
        if entity.type() == 0:  # EntityType.Line
            try:
                if entity.length() < 0.001:  # less than 0.001mm
                    to_remove.append(entity_id)
                    continue
            except:
                pass

        # Remove entities on frozen layers
        layer = doc.layers.layer(entity.layer_name)
        if layer and not layer.visible:
            to_remove.append(entity_id)
            continue

    # Note: actual removal should go through Command system for Undo support
    # This is a demonstration — in production, return the list of IDs to remove
    return {
        "to_remove": to_remove,
        "count": len(to_remove),
        "message": f"정리 대상: {len(to_remove)}개 엔티티 발견"
    }
