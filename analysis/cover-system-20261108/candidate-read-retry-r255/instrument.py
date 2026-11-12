"""Discard one strictly rejected pre-input candidate and read it afresh once.

No reader barrier is relaxed. This applies before original A and the Native
path attempt, never to an input action, query dispatch or unknown result.
"""
BEFORE = """        for address in hints:
            q = CoverQuery(observer)
            link = q.obj(address)
            link_world = q.pointer(link, 'WorldInfo')
            if link['name'].startswith('Default__') or not link_world or link_world['address'] != ctx.world:
                continue
            position = _geometry(q, link, 'Location', 'Vector')['value']
            if math.dist(position, origin) > 1500:
                continue
            rotation = _geometry(q, link, 'Rotation', 'Rotator')['value']
            _, _, _, count = q.array(link, 'Slots', 256)
            assert 0 < count <= 32, ('Cover selection slot budget', count)
            slots = slot_snapshot(q, link, list(range(count)))
            composed = slot_world_candidates(position, rotation, slots['rows'])['rows']
            try:
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
HELPER = '''
def _cover_candidate_retryable(query, error):
    # Eligibility for a NEW read only, never acceptance of the rejected sample.
    if not isinstance(error, RuntimeError) or str(error) != 'World-state metadata/registry changed at closing boundary':
        return False
    difference = query.closing_rejection
    if not isinstance(difference, dict) or difference.get('kind') != 'registry' or difference.get('size') != 16:
        return False
    try:
        objects_header = query.r.objects_header
        if not isinstance(objects_header, int) or objects_header <= 0 or difference.get('address') != objects_header:
            return False
        before = bytes.fromhex(difference['before'])
        after = bytes.fromhex(difference['after'])
        if len(before) != 16 or len(after) != 16:
            return False
        old_data, old_count, old_capacity = struct.unpack('<Qii', before)
        new_data, new_count, new_capacity = struct.unpack('<Qii', after)
        return (old_data != 0 and old_data % 8 == 0 and new_data == old_data
                and old_capacity == new_capacity and 0 < old_count
                and new_count == old_count + 1 and new_count <= new_capacity <= 4000000)
    except (KeyError, TypeError, ValueError, AttributeError, struct.error):
        return False

'''
AFTER = """        for address in hints:
            fresh_candidate = None
            for candidate_read_attempt in range(2):
                q = CoverQuery(observer)
                link = q.obj(address)
                link_world = q.pointer(link, 'WorldInfo')
                if link['name'].startswith('Default__') or not link_world or link_world['address'] != ctx.world:
                    break
                position = _geometry(q, link, 'Location', 'Vector')['value']
                if math.dist(position, origin) > 1500:
                    break
                rotation = _geometry(q, link, 'Rotation', 'Rotator')['value']
                _, _, _, count = q.array(link, 'Slots', 256)
                assert 0 < count <= 32, ('Cover selection slot budget', count)
                slots = slot_snapshot(q, link, list(range(count)))
                composed = slot_world_candidates(position, rotation, slots['rows'])['rows']
                try:
                    q.finish()
                except BaseException as candidate_error:
                    refusal = dict(link_address=address, stage='candidate_selection_before_original_A',
                        difference=q.closing_rejection, difference_missing_is_unknown=q.closing_rejection is None,
                        read_attempt=candidate_read_attempt, retry_next_read=False)
                    report['candidate_closing_rejection'] = refusal
                    report.setdefault('candidate_read_rejections', []).append(refusal)
                    retry = (candidate_read_attempt == 0 and not report.get('candidate_read_retry_used', False)
                             and _cover_candidate_retryable(q, candidate_error))
                    if retry:
                        report['candidate_read_retry_used'] = True
                        refusal['retry_next_read'] = True
                    save()
                    if not retry:
                        raise
                    observer.verify()
                    continue
                fresh_candidate = (link, slots, composed)
                break
            if fresh_candidate is None:
                continue
            link, slots, composed = fresh_candidate
            for raw, computed in zip(slots['rows'], composed):"""

def instrument(source):
    marker = 'import json, math, struct, sys, time\n'
    if source.count(BEFORE) != 1 or source.count(marker) != 1 or '_cover_candidate_retryable' in source:
        raise ValueError('Pre-input candidate boundary changed or already instrumented')
    return source.replace(marker, marker + HELPER).replace(BEFORE, AFTER)
