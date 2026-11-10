"""Record the last ordinary-approach state before the sole original A action."""
NEEDLE="""        try:
            g.tap('buttonA', 1)
        finally:
            g.pad(0, wait=.8)"""
ADDED="""        report['before_original_A_observation_begin_ns'] = time.perf_counter_ns()
        report['before_original_A'] = sample_playground(observer, g.ROOT, ctx.pc, ctx.pawn)
        report['before_original_A_observation_end_ns'] = time.perf_counter_ns()
        save()
"""
def instrument(source):
    if source.count(NEEDLE)!=1 or 'before_original_A_observation_begin_ns' in source:
        raise ValueError('Ordinary A source differs or was already instrumented')
    # The read and durable save occur after the existing .15 forward interval.
    # They add elapsed time while that input remains held; retain the bracket
    # instead of pretending this is an identical timing comparison. No retry,
    # position/yaw setter, extra button edge or altered input deadline is added.
    return source.replace(NEEDLE,ADDED+NEEDLE)
