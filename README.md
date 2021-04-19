## Building

Run `make`.

(TODO fill in more instructions here)

## Example usage

One of the easiest examples is `leader-election.ivy`. You can try it out like so:

    ./run.sh benchmarks/leader-election.ivy --config basic_f --threads 1 --minimal-models --with-conjs

The `--config` option specifies a search space, here `basic_f`,
declared in the adjacent file `leader-election.config` (along with alternate configs).
For this protocol, `basic_f` is one of the fastest configurations.

The `--threads` option specifies the number of concurrent processes to run. Finally, 
`--minimal-models` and `--with-conjs` are additional algorithm options, both recommended. (See below.)

If everything is set up right, it should succeed rather quickly, printing `Success: True`.
The output should include a log directory, something like `logs/log.2021-04-19_15.02.42-710190115`.
To find the invariants that SWISS synthesized, check out the produced `invariants` file.

    # use the log directory from the program output
    $ cat logs/log.2021-04-19_15.02.42-710190115/invariants

You'll see something like,

    conjecture (forall A000:node, A001:node, A002:node . ((((~(btw(A000, A001, A002))) & (~((nid(A000) = nid(A001))))) | ((~(leader(A001))) & (~(pnd(nid(A001), A000)))) | le(nid(A002), nid(A001)))))

## Setting up a benchmark

To create a benchmark, create a `.ivy` file. The `.ivy` file should start with the line `#ivy 1.5` or `#ivy 1.6`.
(Unfortunately, SWISS doesn't support later version of IVy right now).
SWISS also supports the [.pyv](https://github.com/wilcoxjay/mypyvy) format.
Your input file should contain conjectures that represent the safety condition.

Next, create a config file (same directory, same filename with the extension replaced by `.config`).
Here's an example configuration (`benchmarks/paxos.config`):

    [bench basic]

      [breadth]
      template forall node value value quorum round round round # d=1 k=4                                
      template forall round value . exists quorum . forall node node  # d=1 k=3                          

      [finisher]
      template forall round round value value quorum . exists node # d=2 k=6
      
    [bench auto] 

      [breadth]
      auto # d=1 k=3 mvars=5 e=1                                                                         

      [finisher] 
      auto # d=2 k=6 mvars=6 e=1

Each configuration includes a `breadth` config, a `finisher` config, or both. Generally, you'll want to run
both for best results. (A description of these phases can be found in our paper, below.)
For any phase, the config can either be a single `auto` line
or multiple `template` lines.

The template line specifies a template for first-order logical predicates. The `d` and `k` parameters are mandatory.

 * `k`: maximum number of terms in the disjunction
 * `d`: maximum "depth" of predicate as a syntax tree of conjunction/disjunctions. (Only supported values are `1` and `2`.)

So for example,
`template forall node value value quorum round round round # d=1 k=4` would represent the space of logical predicates of the form
`forall N:node, V1:value, V2:value, Q:quorum, R1:round, R2:round, R3:round . a | b | c | d`.

An `auto` config will generate these templates automatically, given the constraints. `k` and `d` are as above, while we also have

 * `mvars`: maximum number of quantified variables (including both universal and existential quantifications)
 * `e`: maximum number of existentially quantified variables

All the templates are given in a _fixed_ nesting order of the sorts, so that the resulting verification
conditions will be in [EPR](https://en.wikipedia.org/wiki/Bernays%E2%80%93Sch%C3%B6nfinkel_class).
This nesting order is determined by the declaration order of the sorts in the `.ivy` or `.pyv` file.

## Publications

Travis Hance, Marijn Heule, Ruben Martins, and Bryan Parno.
_Finding Invariants of Distributed Systems: It’s a Small (Enough) World After All._
NSDI 2021.
