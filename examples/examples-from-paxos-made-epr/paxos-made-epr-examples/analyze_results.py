from numpy import std, mean

rs = [
    eval(open('results.dat').read()),
]
results = {}
for r in rs:
    for k in r.keys():
        if k not in results:
            results[k] = {}
        results[k].update(r[k])

names = ['paxos', 'multi_paxos', 'vertical_paxos', 'fast_paxos', 'flexible_paxos', 'stoppable_paxos']
exts = ['_epr.ivy', '_aux_inv.ivy', '_epr_rewrite_verify.ivy'] + ['_fol_bmc_{}.ivy'.format(n) for n in [2,4,8,16]]

for name in names:
    print 'Timing results for {}:\n'.format(name)
    for ext in exts:
        if ext in results[name]:
            times = results[name][ext]
            #print '{}: mean = {}, std = {}, times = {}'.format(ext, mean(times), std(times), times)
            print '{:30}: mean = {:5.1f}, std = {:5.1f}, timeouts = {}, times: {}'.format(
                ext,
                mean(times),
                std(times),
                len([t for t in times if t >= 300]),
                ', '.join('{:5.1f}'.format(t) for t in times)
            )
    print

titles = {
    'paxos': 'Paxos',
    'multi_paxos': 'Multi-Paxos',
    'vertical_paxos': 'Vertical Paxos',
    'fast_paxos': 'Fast Paxos',
    'flexible_paxos': 'Flexible Paxos',
    'stoppable_paxos': 'Stoppable Paxos',
}
print

def format_times(times):
    if times is None:
        return r'\tna & \tna & \tna'
    else:
        return r'{:.1f} & {:.1f} & {}'.format(
            mean(times),
            std(times),
            len([t for t in times if t >= 300])
        )

for name in names:
    print r"""    \hhline{{||----------------------||}}
    \mbox{{{}}} &
    {}
    \\""".format(
    titles[name],
    ' &\n    '.join(
        format_times(results[name].get(ext))
        for ext in ['_epr.ivy', '_aux_inv.ivy', '_epr_rewrite_verify.ivy'] + ['_fol_bmc_{}.ivy'.format(n) for n in [2,4,8,16]]
    ),
    )
