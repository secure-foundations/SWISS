(set-option :incremental false)
(set-info :status unsat)
(set-logic QF_BV)
(declare-fun v1 () (_ BitVec 4))
(check-sat-assuming ( (bvugt (bvmul ((_ zero_extend 3) (bvcomp (_ bv0 4) ((_ zero_extend 3) (ite (= (_ bv14 4) ((_ sign_extend 3) (ite (bvslt v1 v1) (_ bv1 1) (_ bv0 1)))) (_ bv1 1) (_ bv0 1))))) (bvmul (_ bv8 4) ((_ repeat 1) (_ bv14 4)))) (_ bv0 4)) ))