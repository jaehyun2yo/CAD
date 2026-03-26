"""도무송 파이프라인 테스트"""
import sys
import os

# Add build output to path for yjcad_core
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'build', 'Debug'))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

import yjcad_core as cad
from plugins.domusong.pipeline import Pipeline, PipelineConfig, StepStatus
from plugins.domusong.auto_saver import generate_save_path, get_next_seq
from plugins.domusong.info_text import generate_info_text
from plugins.domusong.outline_generator import generate_outline


def test_pipeline_creation():
    doc = cad.Document()
    config = PipelineConfig(company='원컴퍼니', drawing_name='테스트도면', jeolsu='2절')
    pipe = Pipeline(doc, config)
    assert pipe.total_steps == 10
    assert pipe.progress == 0.0
    assert not pipe.is_complete
    print("✅ Pipeline creation OK")


def test_pipeline_step_execution():
    doc = cad.Document()
    # Add some test entities
    line = cad.LineEntity(doc.next_id(), cad.Point2D(0, 0), cad.Point2D(100, 50))
    # Note: can't add to doc from Python yet (needs addEntity binding)
    # But we can test the pipeline structure

    config = PipelineConfig(company='테스트업체', drawing_name='샘플')
    pipe = Pipeline(doc, config)

    # Run cleanup step
    step = pipe.run_step()
    assert step.name == 'cleanup'
    assert step.status == StepStatus.COMPLETED
    print(f"✅ Step 1 ({step.name}): {step.status.value}")

    # Run entity classifier
    step = pipe.run_step()
    assert step.name == 'entity_classifier'
    assert step.status == StepStatus.COMPLETED
    print(f"✅ Step 2 ({step.name}): {step.status.value}")


def test_pipeline_skip():
    doc = cad.Document()
    config = PipelineConfig()
    pipe = Pipeline(doc, config)

    pipe.skip_step()  # Skip cleanup
    assert pipe.steps[0].status == StepStatus.SKIPPED
    assert pipe.current_step == 1
    print("✅ Pipeline skip OK")


def test_pipeline_summary():
    doc = cad.Document()
    config = PipelineConfig(company='테스트')
    pipe = Pipeline(doc, config)

    pipe.run_step()
    pipe.skip_step()
    pipe.run_step()

    summary = pipe.summary()
    assert summary['completed'] == 2
    assert summary['skipped'] == 1
    assert summary['total'] == 10
    print(f"✅ Pipeline summary: {summary['progress']}")


def test_auto_saver():
    path = generate_save_path('/tmp/dxf', 1, '원컴퍼니', '퀘이크업미니가차', '2절', '0326')
    assert path.endswith('0326-1 원컴퍼니 퀘이크업미니가차 2절.DXF')
    print(f"✅ Auto-save path: {os.path.basename(path)}")


def test_info_text():
    doc = cad.Document()
    result = generate_info_text(doc, '원컴퍼니', '테스트도면', 'A4', '03/26')
    assert '원컴퍼니' in result['text']
    assert '테스트도면' in result['text']
    assert result['height'] == 5.0
    print(f"✅ Info text: {result['text']}")


def test_outline_generator():
    doc = cad.Document()
    result = generate_outline(doc, '동판', '2절')
    assert len(result['vertices']) == 4
    assert result['closed'] is True
    assert result['outline_type'] == '동판'
    print(f"✅ Outline: {result['message']}")


if __name__ == '__main__':
    test_pipeline_creation()
    test_pipeline_step_execution()
    test_pipeline_skip()
    test_pipeline_summary()
    test_auto_saver()
    test_info_text()
    test_outline_generator()
    print("\n=== 도무송 플러그인 테스트 전부 통과! ===")
