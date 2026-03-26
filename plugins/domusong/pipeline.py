"""
도무송 자동화: pipeline
10단계 가공 파이프라인 순차 실행

파이프라인 단계:
  1. cleanup — 불필요한 엔티티 제거
  2. entity_classifier — 오시/칼선 자동 분류 (반자동)
  3. bridge_inserter — 칼선에 브릿지 삽입
  4. arc_to_lines — R값 선분잇기 (점빼기)
  5. orientation — 정타/후타 판단 + 뒤집기 (반자동)
  6. cut_line — 씨아기칼 생성
  7. outline — 외곽선 생성
  8. guide_line — 구아이선 삽입
  9. info_text — 정보 텍스트 작성
  10. auto_save — 파일명 규칙 저장

각 단계는 독립적으로 실행 가능하며, 실패 시 중단/건너뜀 선택 가능.
"""

from enum import Enum
from dataclasses import dataclass, field
from typing import List, Optional, Callable, Any


class StepStatus(Enum):
    PENDING = 'pending'
    RUNNING = 'running'
    COMPLETED = 'completed'
    SKIPPED = 'skipped'
    FAILED = 'failed'


@dataclass
class PipelineStep:
    name: str
    description: str
    function: Optional[Callable] = None
    status: StepStatus = StepStatus.PENDING
    result: Any = None
    error: str = ''
    requires_confirmation: bool = False  # 반자동 단계


@dataclass
class PipelineConfig:
    """파이프라인 실행 설정"""
    company: str = ''
    drawing_name: str = ''
    jeolsu: str = ''
    print_size: str = ''
    orientation: str = '정타'  # 정타 or 후타
    bridge_width: float = 3.0  # mm
    bridge_spacing: float = 20.0  # mm
    arc_tolerance: float = 0.1  # mm
    outline_type: str = '일반'
    save_dir: str = ''


class Pipeline:
    """도무송 가공 파이프라인 — 10단계 순차 실행"""

    def __init__(self, doc, config: PipelineConfig):
        self.doc = doc
        self.config = config
        self.steps: List[PipelineStep] = self._create_steps()
        self.current_step = 0

    def _create_steps(self) -> List[PipelineStep]:
        return [
            PipelineStep('cleanup', '불필요한 엔티티 제거'),
            PipelineStep('entity_classifier', '오시/칼선 분류',
                         requires_confirmation=True),
            PipelineStep('bridge_inserter', '브릿지 삽입'),
            PipelineStep('arc_to_lines', 'R값 선분잇기 (점빼기)'),
            PipelineStep('orientation', '정타/후타 텍스트 + 뒤집기',
                         requires_confirmation=True),
            PipelineStep('cut_line', '씨아기칼 생성'),
            PipelineStep('outline', '외곽선 생성'),
            PipelineStep('guide_line', '구아이선 삽입'),
            PipelineStep('info_text', '정보 텍스트 작성'),
            PipelineStep('auto_save', '파일명 규칙 저장'),
        ]

    @property
    def total_steps(self) -> int:
        return len(self.steps)

    @property
    def progress(self) -> float:
        """0.0 ~ 1.0"""
        completed = sum(1 for s in self.steps
                        if s.status in (StepStatus.COMPLETED, StepStatus.SKIPPED))
        return completed / self.total_steps

    @property
    def is_complete(self) -> bool:
        return self.current_step >= self.total_steps

    def current(self) -> Optional[PipelineStep]:
        if self.current_step < self.total_steps:
            return self.steps[self.current_step]
        return None

    def run_step(self) -> PipelineStep:
        """현재 단계를 실행하고 다음으로 이동.

        Returns:
            실행된 PipelineStep (status가 업데이트됨)
        """
        if self.is_complete:
            return None

        step = self.steps[self.current_step]
        step.status = StepStatus.RUNNING

        try:
            result = self._execute_step(step)
            step.result = result
            step.status = StepStatus.COMPLETED
        except Exception as e:
            step.error = str(e)
            step.status = StepStatus.FAILED

        self.current_step += 1
        return step

    def skip_step(self) -> PipelineStep:
        """현재 단계를 건너뛴다."""
        if self.is_complete:
            return None
        step = self.steps[self.current_step]
        step.status = StepStatus.SKIPPED
        self.current_step += 1
        return step

    def _execute_step(self, step: PipelineStep):
        """단계별 실행 로직"""
        name = step.name

        if name == 'cleanup':
            from . import cleanup
            return cleanup.run(self.doc)

        elif name == 'entity_classifier':
            from . import entity_classifier
            return entity_classifier.classify(self.doc)

        elif name == 'bridge_inserter':
            # Bridge insertion is done through C++ GeometryOps
            return {'message': '브릿지 삽입 — C++ API 호출 필요',
                    'bridge_width': self.config.bridge_width,
                    'spacing': self.config.bridge_spacing}

        elif name == 'arc_to_lines':
            return {'message': '점빼기 — C++ API 호출 필요',
                    'tolerance': self.config.arc_tolerance}

        elif name == 'orientation':
            return {'orientation': self.config.orientation,
                    'message': f'{self.config.orientation} — 텍스트 + 뒤집기'}

        elif name == 'cut_line':
            return {'message': '씨아기칼 생성',
                    'print_size': self.config.print_size}

        elif name == 'outline':
            from . import outline_generator
            return outline_generator.generate_outline(
                self.doc, self.config.outline_type, self.config.jeolsu)

        elif name == 'guide_line':
            return {'message': '구아이선 삽입 — C++ API 호출 필요'}

        elif name == 'info_text':
            from . import info_text
            return info_text.generate_info_text(
                self.doc, self.config.company,
                self.config.drawing_name, self.config.print_size)

        elif name == 'auto_save':
            from . import auto_saver
            if self.config.save_dir:
                seq = auto_saver.get_next_seq(self.config.save_dir)
                path = auto_saver.generate_save_path(
                    self.config.save_dir, seq,
                    self.config.company, self.config.drawing_name,
                    self.config.jeolsu)
                return {'save_path': path, 'seq': seq,
                        'message': f'저장 경로: {path}'}
            return {'message': '저장 폴더 미설정'}

        return {'message': f'미구현 단계: {name}'}

    def summary(self) -> dict:
        """파이프라인 실행 결과 요약"""
        return {
            'total': self.total_steps,
            'completed': sum(1 for s in self.steps if s.status == StepStatus.COMPLETED),
            'skipped': sum(1 for s in self.steps if s.status == StepStatus.SKIPPED),
            'failed': sum(1 for s in self.steps if s.status == StepStatus.FAILED),
            'pending': sum(1 for s in self.steps if s.status == StepStatus.PENDING),
            'progress': f'{self.progress * 100:.0f}%',
            'steps': [
                {'name': s.name, 'status': s.status.value,
                 'message': s.result.get('message', '') if isinstance(s.result, dict) else '',
                 'error': s.error}
                for s in self.steps
            ]
        }
