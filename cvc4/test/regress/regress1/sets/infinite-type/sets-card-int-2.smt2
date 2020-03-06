(set-logic QF_ALL)
(set-option :sets-ext true)
(set-info :status sat)
(assert (= (card (as univset (Set Int))) 10))
(declare-const universe (Set Int))
(declare-const A (Set Int))
(declare-const B (Set Int))
(assert (= (card A) 5))
(assert (= (card B) 5))
(assert (= (intersection A B) (as emptyset (Set Int))))
(assert (= universe (as univset (Set Int))))
(check-sat)
