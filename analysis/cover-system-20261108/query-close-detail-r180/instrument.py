"""Persist the existing strict candidate-closing refusal, without another read."""
NEEDLE = """            q.finish()
            for raw, computed in zip(slots['rows'], composed):"""
REPLACEMENT = """            try:
                q.finish()
            except BaseException:
                report['candidate_closing_rejection'] = dict(
                    link_address=address,
                    stage='candidate_selection_before_original_A',
                    difference=q.closing_rejection,
                    difference_missing_is_unknown=q.closing_rejection is None)
                save()
                raise
            for raw, computed in zip(slots['rows'], composed):"""


def instrument(source):
    if source.count(NEEDLE) != 1 or 'candidate_closing_rejection' in source:
        raise ValueError('Candidate close boundary changed or already instrumented')
    return source.replace(NEEDLE, REPLACEMENT)
