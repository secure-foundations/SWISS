(set-option :incremental false)
(set-info :status sat)
(set-logic QF_BV)
(declare-fun v0 () (_ BitVec 4))
(check-sat-assuming ( (= ((_ extract 0 0) (bvadd (bvnot v0) (_ bv1 4))) (_ bv0 1)) ))
