def axiom_for_sort(letter, sort, n):
    return ''.join ([
        'axiom exists ',
        ','.join('{}{}:{}'.format(letter,i,sort) for i in range(1,n+1)),
        '. forall {}:{}. '.format(letter,sort),
        ' | '.join('{}={}{}'.format(letter,letter,i) for i in range(1,n+1)),
    ])

def text_for(n):
    return """

# restrict size of domain (auto generated for {} rounds)
{}
{}
""".format(n,
           axiom_for_sort('V', 'value', 2),
           axiom_for_sort('R', 'round', n),
       )

for name in ['paxos', 'multi_paxos', 'vertical_paxos', 'fast_paxos', 'flexible_paxos', 'stoppable_paxos']:
    src_fn = '{}/{}_fol.ivy'.format(name,name)
    for n in [2,4,8,16]:
        dst_fn = '{}/{}_fol_bmc_{}.ivy'.format(name,name,n)
        open(dst_fn, 'w').write(
            open(src_fn).read() + text_for(n)
        )
