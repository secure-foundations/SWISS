(set-option :incremental false)
(set-info :source "http://www.cs.bham.ac.uk/~vxs/quasigroups/benchmark/")
(set-info :status unsat)
(set-info :difficulty "0")
(set-info :category "crafted")
(set-logic QF_UF)
(declare-sort U 0)
(declare-sort I 0)
(declare-fun op (I I) I)
(declare-fun e4 () I)
(declare-fun e3 () I)
(declare-fun e2 () I)
(declare-fun e1 () I)
(declare-fun e0 () I)
(assert (let ((_let_0 (op e0 e0))) (let ((_let_1 (op e0 e1))) (let ((_let_2 (op e0 e2))) (let ((_let_3 (op e0 e3))) (let ((_let_4 (op e0 e4))) (let ((_let_5 (op e1 e0))) (let ((_let_6 (op e1 e1))) (let ((_let_7 (op e1 e2))) (let ((_let_8 (op e1 e3))) (let ((_let_9 (op e1 e4))) (let ((_let_10 (op e2 e0))) (let ((_let_11 (op e2 e1))) (let ((_let_12 (op e2 e2))) (let ((_let_13 (op e2 e3))) (let ((_let_14 (op e2 e4))) (let ((_let_15 (op e3 e0))) (let ((_let_16 (op e3 e1))) (let ((_let_17 (op e3 e2))) (let ((_let_18 (op e3 e3))) (let ((_let_19 (op e3 e4))) (let ((_let_20 (op e4 e0))) (let ((_let_21 (op e4 e1))) (let ((_let_22 (op e4 e2))) (let ((_let_23 (op e4 e3))) (let ((_let_24 (op e4 e4))) (and (and (and (and (and (and (and (and (or (or (or (or (= _let_0 e0) (= _let_0 e1)) (= _let_0 e2)) (= _let_0 e3)) (= _let_0 e4)) (or (or (or (or (= _let_1 e0) (= _let_1 e1)) (= _let_1 e2)) (= _let_1 e3)) (= _let_1 e4))) (or (or (or (or (= _let_2 e0) (= _let_2 e1)) (= _let_2 e2)) (= _let_2 e3)) (= _let_2 e4))) (or (or (or (or (= _let_3 e0) (= _let_3 e1)) (= _let_3 e2)) (= _let_3 e3)) (= _let_3 e4))) (or (or (or (or (= _let_4 e0) (= _let_4 e1)) (= _let_4 e2)) (= _let_4 e3)) (= _let_4 e4))) (and (and (and (and (or (or (or (or (= _let_5 e0) (= _let_5 e1)) (= _let_5 e2)) (= _let_5 e3)) (= _let_5 e4)) (or (or (or (or (= _let_6 e0) (= _let_6 e1)) (= _let_6 e2)) (= _let_6 e3)) (= _let_6 e4))) (or (or (or (or (= _let_7 e0) (= _let_7 e1)) (= _let_7 e2)) (= _let_7 e3)) (= _let_7 e4))) (or (or (or (or (= _let_8 e0) (= _let_8 e1)) (= _let_8 e2)) (= _let_8 e3)) (= _let_8 e4))) (or (or (or (or (= _let_9 e0) (= _let_9 e1)) (= _let_9 e2)) (= _let_9 e3)) (= _let_9 e4)))) (and (and (and (and (or (or (or (or (= _let_10 e0) (= _let_10 e1)) (= _let_10 e2)) (= _let_10 e3)) (= _let_10 e4)) (or (or (or (or (= _let_11 e0) (= _let_11 e1)) (= _let_11 e2)) (= _let_11 e3)) (= _let_11 e4))) (or (or (or (or (= _let_12 e0) (= _let_12 e1)) (= _let_12 e2)) (= _let_12 e3)) (= _let_12 e4))) (or (or (or (or (= _let_13 e0) (= _let_13 e1)) (= _let_13 e2)) (= _let_13 e3)) (= _let_13 e4))) (or (or (or (or (= _let_14 e0) (= _let_14 e1)) (= _let_14 e2)) (= _let_14 e3)) (= _let_14 e4)))) (and (and (and (and (or (or (or (or (= _let_15 e0) (= _let_15 e1)) (= _let_15 e2)) (= _let_15 e3)) (= _let_15 e4)) (or (or (or (or (= _let_16 e0) (= _let_16 e1)) (= _let_16 e2)) (= _let_16 e3)) (= _let_16 e4))) (or (or (or (or (= _let_17 e0) (= _let_17 e1)) (= _let_17 e2)) (= _let_17 e3)) (= _let_17 e4))) (or (or (or (or (= _let_18 e0) (= _let_18 e1)) (= _let_18 e2)) (= _let_18 e3)) (= _let_18 e4))) (or (or (or (or (= _let_19 e0) (= _let_19 e1)) (= _let_19 e2)) (= _let_19 e3)) (= _let_19 e4)))) (and (and (and (and (or (or (or (or (= _let_20 e0) (= _let_20 e1)) (= _let_20 e2)) (= _let_20 e3)) (= _let_20 e4)) (or (or (or (or (= _let_21 e0) (= _let_21 e1)) (= _let_21 e2)) (= _let_21 e3)) (= _let_21 e4))) (or (or (or (or (= _let_22 e0) (= _let_22 e1)) (= _let_22 e2)) (= _let_22 e3)) (= _let_22 e4))) (or (or (or (or (= _let_23 e0) (= _let_23 e1)) (= _let_23 e2)) (= _let_23 e3)) (= _let_23 e4))) (or (or (or (or (= _let_24 e0) (= _let_24 e1)) (= _let_24 e2)) (= _let_24 e3)) (= _let_24 e4))))))))))))))))))))))))))))))
(assert (let ((_let_0 (op e0 e1))) (let ((_let_1 (op e0 e2))) (let ((_let_2 (op e0 e3))) (let ((_let_3 (op e0 e4))) (let ((_let_4 (op e1 e0))) (let ((_let_5 (op e1 e2))) (let ((_let_6 (op e1 e3))) (let ((_let_7 (op e1 e4))) (let ((_let_8 (op e2 e0))) (let ((_let_9 (op e2 e1))) (let ((_let_10 (op e2 e3))) (let ((_let_11 (op e2 e4))) (let ((_let_12 (op e3 e0))) (let ((_let_13 (op e3 e1))) (let ((_let_14 (op e3 e2))) (let ((_let_15 (op e3 e4))) (let ((_let_16 (op e4 e0))) (let ((_let_17 (op e4 e1))) (let ((_let_18 (op e4 e2))) (let ((_let_19 (op e4 e3))) (let ((_let_20 (= (op e0 e0) e0))) (let ((_let_21 (= (op e0 e0) e1))) (let ((_let_22 (= (op e0 e0) e2))) (let ((_let_23 (= (op e0 e0) e3))) (let ((_let_24 (= (op e0 e0) e4))) (let ((_let_25 (= _let_0 e0))) (let ((_let_26 (= _let_0 e1))) (let ((_let_27 (= _let_0 e2))) (let ((_let_28 (= _let_0 e3))) (let ((_let_29 (= _let_0 e4))) (let ((_let_30 (= _let_1 e0))) (let ((_let_31 (= _let_1 e1))) (let ((_let_32 (= _let_1 e2))) (let ((_let_33 (= _let_1 e3))) (let ((_let_34 (= _let_1 e4))) (let ((_let_35 (= _let_2 e0))) (let ((_let_36 (= _let_2 e1))) (let ((_let_37 (= _let_2 e2))) (let ((_let_38 (= _let_2 e3))) (let ((_let_39 (= _let_2 e4))) (let ((_let_40 (= _let_3 e0))) (let ((_let_41 (= _let_3 e1))) (let ((_let_42 (= _let_3 e2))) (let ((_let_43 (= _let_3 e3))) (let ((_let_44 (= _let_3 e4))) (let ((_let_45 (= _let_4 e0))) (let ((_let_46 (= _let_4 e1))) (let ((_let_47 (= _let_4 e2))) (let ((_let_48 (= _let_4 e3))) (let ((_let_49 (= _let_4 e4))) (let ((_let_50 (= (op e1 e1) e0))) (let ((_let_51 (= (op e1 e1) e1))) (let ((_let_52 (= (op e1 e1) e2))) (let ((_let_53 (= (op e1 e1) e3))) (let ((_let_54 (= (op e1 e1) e4))) (let ((_let_55 (= _let_5 e0))) (let ((_let_56 (= _let_5 e1))) (let ((_let_57 (= _let_5 e2))) (let ((_let_58 (= _let_5 e3))) (let ((_let_59 (= _let_5 e4))) (let ((_let_60 (= _let_6 e0))) (let ((_let_61 (= _let_6 e1))) (let ((_let_62 (= _let_6 e2))) (let ((_let_63 (= _let_6 e3))) (let ((_let_64 (= _let_6 e4))) (let ((_let_65 (= _let_7 e0))) (let ((_let_66 (= _let_7 e1))) (let ((_let_67 (= _let_7 e2))) (let ((_let_68 (= _let_7 e3))) (let ((_let_69 (= _let_7 e4))) (let ((_let_70 (= _let_8 e0))) (let ((_let_71 (= _let_8 e1))) (let ((_let_72 (= _let_8 e2))) (let ((_let_73 (= _let_8 e3))) (let ((_let_74 (= _let_8 e4))) (let ((_let_75 (= _let_9 e0))) (let ((_let_76 (= _let_9 e1))) (let ((_let_77 (= _let_9 e2))) (let ((_let_78 (= _let_9 e3))) (let ((_let_79 (= _let_9 e4))) (let ((_let_80 (= (op e2 e2) e0))) (let ((_let_81 (= (op e2 e2) e1))) (let ((_let_82 (= (op e2 e2) e2))) (let ((_let_83 (= (op e2 e2) e3))) (let ((_let_84 (= (op e2 e2) e4))) (let ((_let_85 (= _let_10 e0))) (let ((_let_86 (= _let_10 e1))) (let ((_let_87 (= _let_10 e2))) (let ((_let_88 (= _let_10 e3))) (let ((_let_89 (= _let_10 e4))) (let ((_let_90 (= _let_11 e0))) (let ((_let_91 (= _let_11 e1))) (let ((_let_92 (= _let_11 e2))) (let ((_let_93 (= _let_11 e3))) (let ((_let_94 (= _let_11 e4))) (let ((_let_95 (= _let_12 e0))) (let ((_let_96 (= _let_12 e1))) (let ((_let_97 (= _let_12 e2))) (let ((_let_98 (= _let_12 e3))) (let ((_let_99 (= _let_12 e4))) (let ((_let_100 (= _let_13 e0))) (let ((_let_101 (= _let_13 e1))) (let ((_let_102 (= _let_13 e2))) (let ((_let_103 (= _let_13 e3))) (let ((_let_104 (= _let_13 e4))) (let ((_let_105 (= _let_14 e0))) (let ((_let_106 (= _let_14 e1))) (let ((_let_107 (= _let_14 e2))) (let ((_let_108 (= _let_14 e3))) (let ((_let_109 (= _let_14 e4))) (let ((_let_110 (= (op e3 e3) e0))) (let ((_let_111 (= (op e3 e3) e1))) (let ((_let_112 (= (op e3 e3) e2))) (let ((_let_113 (= (op e3 e3) e3))) (let ((_let_114 (= (op e3 e3) e4))) (let ((_let_115 (= _let_15 e0))) (let ((_let_116 (= _let_15 e1))) (let ((_let_117 (= _let_15 e2))) (let ((_let_118 (= _let_15 e3))) (let ((_let_119 (= _let_15 e4))) (let ((_let_120 (= _let_16 e0))) (let ((_let_121 (= _let_16 e1))) (let ((_let_122 (= _let_16 e2))) (let ((_let_123 (= _let_16 e3))) (let ((_let_124 (= _let_16 e4))) (let ((_let_125 (= _let_17 e0))) (let ((_let_126 (= _let_17 e1))) (let ((_let_127 (= _let_17 e2))) (let ((_let_128 (= _let_17 e3))) (let ((_let_129 (= _let_17 e4))) (let ((_let_130 (= _let_18 e0))) (let ((_let_131 (= _let_18 e1))) (let ((_let_132 (= _let_18 e2))) (let ((_let_133 (= _let_18 e3))) (let ((_let_134 (= _let_18 e4))) (let ((_let_135 (= _let_19 e0))) (let ((_let_136 (= _let_19 e1))) (let ((_let_137 (= _let_19 e2))) (let ((_let_138 (= _let_19 e3))) (let ((_let_139 (= _let_19 e4))) (let ((_let_140 (= (op e4 e4) e0))) (let ((_let_141 (= (op e4 e4) e1))) (let ((_let_142 (= (op e4 e4) e2))) (let ((_let_143 (= (op e4 e4) e3))) (let ((_let_144 (= (op e4 e4) e4))) (and (and (and (and (and (and (and (and (and (or (or (or (or _let_20 _let_25) _let_30) _let_35) _let_40) (or (or (or (or _let_20 _let_45) _let_70) _let_95) _let_120)) (and (or (or (or (or _let_21 _let_26) _let_31) _let_36) _let_41) (or (or (or (or _let_21 _let_46) _let_71) _let_96) _let_121))) (and (or (or (or (or _let_22 _let_27) _let_32) _let_37) _let_42) (or (or (or (or _let_22 _let_47) _let_72) _let_97) _let_122))) (and (or (or (or (or _let_23 _let_28) _let_33) _let_38) _let_43) (or (or (or (or _let_23 _let_48) _let_73) _let_98) _let_123))) (and (or (or (or (or _let_24 _let_29) _let_34) _let_39) _let_44) (or (or (or (or _let_24 _let_49) _let_74) _let_99) _let_124))) (and (and (and (and (and (or (or (or (or _let_45 _let_50) _let_55) _let_60) _let_65) (or (or (or (or _let_25 _let_50) _let_75) _let_100) _let_125)) (and (or (or (or (or _let_46 _let_51) _let_56) _let_61) _let_66) (or (or (or (or _let_26 _let_51) _let_76) _let_101) _let_126))) (and (or (or (or (or _let_47 _let_52) _let_57) _let_62) _let_67) (or (or (or (or _let_27 _let_52) _let_77) _let_102) _let_127))) (and (or (or (or (or _let_48 _let_53) _let_58) _let_63) _let_68) (or (or (or (or _let_28 _let_53) _let_78) _let_103) _let_128))) (and (or (or (or (or _let_49 _let_54) _let_59) _let_64) _let_69) (or (or (or (or _let_29 _let_54) _let_79) _let_104) _let_129)))) (and (and (and (and (and (or (or (or (or _let_70 _let_75) _let_80) _let_85) _let_90) (or (or (or (or _let_30 _let_55) _let_80) _let_105) _let_130)) (and (or (or (or (or _let_71 _let_76) _let_81) _let_86) _let_91) (or (or (or (or _let_31 _let_56) _let_81) _let_106) _let_131))) (and (or (or (or (or _let_72 _let_77) _let_82) _let_87) _let_92) (or (or (or (or _let_32 _let_57) _let_82) _let_107) _let_132))) (and (or (or (or (or _let_73 _let_78) _let_83) _let_88) _let_93) (or (or (or (or _let_33 _let_58) _let_83) _let_108) _let_133))) (and (or (or (or (or _let_74 _let_79) _let_84) _let_89) _let_94) (or (or (or (or _let_34 _let_59) _let_84) _let_109) _let_134)))) (and (and (and (and (and (or (or (or (or _let_95 _let_100) _let_105) _let_110) _let_115) (or (or (or (or _let_35 _let_60) _let_85) _let_110) _let_135)) (and (or (or (or (or _let_96 _let_101) _let_106) _let_111) _let_116) (or (or (or (or _let_36 _let_61) _let_86) _let_111) _let_136))) (and (or (or (or (or _let_97 _let_102) _let_107) _let_112) _let_117) (or (or (or (or _let_37 _let_62) _let_87) _let_112) _let_137))) (and (or (or (or (or _let_98 _let_103) _let_108) _let_113) _let_118) (or (or (or (or _let_38 _let_63) _let_88) _let_113) _let_138))) (and (or (or (or (or _let_99 _let_104) _let_109) _let_114) _let_119) (or (or (or (or _let_39 _let_64) _let_89) _let_114) _let_139)))) (and (and (and (and (and (or (or (or (or _let_120 _let_125) _let_130) _let_135) _let_140) (or (or (or (or _let_40 _let_65) _let_90) _let_115) _let_140)) (and (or (or (or (or _let_121 _let_126) _let_131) _let_136) _let_141) (or (or (or (or _let_41 _let_66) _let_91) _let_116) _let_141))) (and (or (or (or (or _let_122 _let_127) _let_132) _let_137) _let_142) (or (or (or (or _let_42 _let_67) _let_92) _let_117) _let_142))) (and (or (or (or (or _let_123 _let_128) _let_133) _let_138) _let_143) (or (or (or (or _let_43 _let_68) _let_93) _let_118) _let_143))) (and (or (or (or (or _let_124 _let_129) _let_134) _let_139) _let_144) (or (or (or (or _let_44 _let_69) _let_94) _let_119) _let_144))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))
(assert (let ((_let_0 (op e0 e0))) (let ((_let_1 (op e1 e1))) (let ((_let_2 (op e2 e2))) (let ((_let_3 (op e3 e3))) (let ((_let_4 (op e4 e4))) (and (and (and (and (or (or (or (or (= _let_0 e0) (= _let_1 e0)) (= _let_2 e0)) (= _let_3 e0)) (= _let_4 e0)) (or (or (or (or (= _let_0 e1) (= _let_1 e1)) (= _let_2 e1)) (= _let_3 e1)) (= _let_4 e1))) (or (or (or (or (= _let_0 e2) (= _let_1 e2)) (= _let_2 e2)) (= _let_3 e2)) (= _let_4 e2))) (or (or (or (or (= _let_0 e3) (= _let_1 e3)) (= _let_2 e3)) (= _let_3 e3)) (= _let_4 e3))) (or (or (or (or (= _let_0 e4) (= _let_1 e4)) (= _let_2 e4)) (= _let_3 e4)) (= _let_4 e4)))))))))
(assert (and (and (and (and (or (or (or (or (= (op e0 e0) e0) (= (op e1 e0) e1)) (= (op e2 e0) e2)) (= (op e3 e0) e3)) (= (op e4 e0) e4)) (or (or (or (or (= (op e0 e1) e0) (= (op e1 e1) e1)) (= (op e2 e1) e2)) (= (op e3 e1) e3)) (= (op e4 e1) e4))) (or (or (or (or (= (op e0 e2) e0) (= (op e1 e2) e1)) (= (op e2 e2) e2)) (= (op e3 e2) e3)) (= (op e4 e2) e4))) (or (or (or (or (= (op e0 e3) e0) (= (op e1 e3) e1)) (= (op e2 e3) e2)) (= (op e3 e3) e3)) (= (op e4 e3) e4))) (or (or (or (or (= (op e0 e4) e0) (= (op e1 e4) e1)) (= (op e2 e4) e2)) (= (op e3 e4) e3)) (= (op e4 e4) e4))))
(assert (let ((_let_0 (op e0 e0))) (let ((_let_1 (op e0 e1))) (let ((_let_2 (op e0 e2))) (let ((_let_3 (op e0 e3))) (let ((_let_4 (op e0 e4))) (let ((_let_5 (op e1 e0))) (let ((_let_6 (op e1 e1))) (let ((_let_7 (op e1 e2))) (let ((_let_8 (op e1 e3))) (let ((_let_9 (op e1 e4))) (let ((_let_10 (op e2 e0))) (let ((_let_11 (op e2 e1))) (let ((_let_12 (op e2 e2))) (let ((_let_13 (op e2 e3))) (let ((_let_14 (op e2 e4))) (let ((_let_15 (op e3 e0))) (let ((_let_16 (op e3 e1))) (let ((_let_17 (op e3 e2))) (let ((_let_18 (op e3 e3))) (let ((_let_19 (op e3 e4))) (let ((_let_20 (op e4 e0))) (let ((_let_21 (op e4 e1))) (let ((_let_22 (op e4 e2))) (let ((_let_23 (op e4 e3))) (let ((_let_24 (op e4 e4))) (or (or (or (or (or (or (or (or (not (= _let_0 _let_0)) (not (= _let_5 _let_1))) (not (= _let_10 _let_2))) (not (= _let_15 _let_3))) (not (= _let_20 _let_4))) (or (or (or (or (not (= _let_1 _let_5)) (not (= _let_6 _let_6))) (not (= _let_11 _let_7))) (not (= _let_16 _let_8))) (not (= _let_21 _let_9)))) (or (or (or (or (not (= _let_2 _let_10)) (not (= _let_7 _let_11))) (not (= _let_12 _let_12))) (not (= _let_17 _let_13))) (not (= _let_22 _let_14)))) (or (or (or (or (not (= _let_3 _let_15)) (not (= _let_8 _let_16))) (not (= _let_13 _let_17))) (not (= _let_18 _let_18))) (not (= _let_23 _let_19)))) (or (or (or (or (not (= _let_4 _let_20)) (not (= _let_9 _let_21))) (not (= _let_14 _let_22))) (not (= _let_19 _let_23))) (not (= _let_24 _let_24))))))))))))))))))))))))))))))
(assert (and (and (and (and (not (= (op e0 e0) e0)) (not (= (op e1 e1) e1))) (not (= (op e2 e2) e2))) (not (= (op e3 e3) e3))) (not (= (op e4 e4) e4))))
(assert (let ((_let_0 (= (op e0 (op e0 e0)) e0))) (let ((_let_1 (= (op e1 (op e1 e1)) e1))) (let ((_let_2 (= (op e2 (op e2 e2)) e2))) (let ((_let_3 (= (op e3 (op e3 e3)) e3))) (let ((_let_4 (= (op e4 (op e4 e4)) e4))) (and (and (and (and (and (not _let_0) (not _let_1)) (not _let_2)) (not _let_3)) (not _let_4)) (and (and (and (and (and (and (and (and _let_0 (= (op e0 (op e0 e1)) e1)) (= (op e0 (op e0 e2)) e2)) (= (op e0 (op e0 e3)) e3)) (= (op e0 (op e0 e4)) e4)) (and (and (and (and (= (op e1 (op e1 e0)) e0) _let_1) (= (op e1 (op e1 e2)) e2)) (= (op e1 (op e1 e3)) e3)) (= (op e1 (op e1 e4)) e4))) (and (and (and (and (= (op e2 (op e2 e0)) e0) (= (op e2 (op e2 e1)) e1)) _let_2) (= (op e2 (op e2 e3)) e3)) (= (op e2 (op e2 e4)) e4))) (and (and (and (and (= (op e3 (op e3 e0)) e0) (= (op e3 (op e3 e1)) e1)) (= (op e3 (op e3 e2)) e2)) _let_3) (= (op e3 (op e3 e4)) e4))) (and (and (and (and (= (op e4 (op e4 e0)) e0) (= (op e4 (op e4 e1)) e1)) (= (op e4 (op e4 e2)) e2)) (= (op e4 (op e4 e3)) e3)) _let_4)))))))))
(assert (let ((_let_0 (op e0 e0))) (let ((_let_1 (op e0 e1))) (let ((_let_2 (op e0 e2))) (let ((_let_3 (op e0 e3))) (let ((_let_4 (op e0 e4))) (let ((_let_5 (op e1 e0))) (let ((_let_6 (op e1 e1))) (let ((_let_7 (op e1 e2))) (let ((_let_8 (op e1 e3))) (let ((_let_9 (op e1 e4))) (let ((_let_10 (op e2 e0))) (let ((_let_11 (op e2 e1))) (let ((_let_12 (op e2 e2))) (let ((_let_13 (op e2 e3))) (let ((_let_14 (op e2 e4))) (let ((_let_15 (op e3 e0))) (let ((_let_16 (op e3 e1))) (let ((_let_17 (op e3 e2))) (let ((_let_18 (op e3 e3))) (let ((_let_19 (op e3 e4))) (let ((_let_20 (op e4 e0))) (let ((_let_21 (op e4 e1))) (let ((_let_22 (op e4 e2))) (let ((_let_23 (op e4 e3))) (let ((_let_24 (op e4 e4))) (and (and (and (and (and (and (and (and (and (and (and (and (and (and (not (= _let_0 _let_5)) (not (= _let_0 _let_10))) (not (= _let_0 _let_15))) (not (= _let_0 _let_20))) (not (= _let_5 _let_10))) (not (= _let_5 _let_15))) (not (= _let_5 _let_20))) (not (= _let_10 _let_15))) (not (= _let_10 _let_20))) (not (= _let_15 _let_20))) (and (and (and (and (and (and (and (and (and (not (= _let_1 _let_6)) (not (= _let_1 _let_11))) (not (= _let_1 _let_16))) (not (= _let_1 _let_21))) (not (= _let_6 _let_11))) (not (= _let_6 _let_16))) (not (= _let_6 _let_21))) (not (= _let_11 _let_16))) (not (= _let_11 _let_21))) (not (= _let_16 _let_21)))) (and (and (and (and (and (and (and (and (and (not (= _let_2 _let_7)) (not (= _let_2 _let_12))) (not (= _let_2 _let_17))) (not (= _let_2 _let_22))) (not (= _let_7 _let_12))) (not (= _let_7 _let_17))) (not (= _let_7 _let_22))) (not (= _let_12 _let_17))) (not (= _let_12 _let_22))) (not (= _let_17 _let_22)))) (and (and (and (and (and (and (and (and (and (not (= _let_3 _let_8)) (not (= _let_3 _let_13))) (not (= _let_3 _let_18))) (not (= _let_3 _let_23))) (not (= _let_8 _let_13))) (not (= _let_8 _let_18))) (not (= _let_8 _let_23))) (not (= _let_13 _let_18))) (not (= _let_13 _let_23))) (not (= _let_18 _let_23)))) (and (and (and (and (and (and (and (and (and (not (= _let_4 _let_9)) (not (= _let_4 _let_14))) (not (= _let_4 _let_19))) (not (= _let_4 _let_24))) (not (= _let_9 _let_14))) (not (= _let_9 _let_19))) (not (= _let_9 _let_24))) (not (= _let_14 _let_19))) (not (= _let_14 _let_24))) (not (= _let_19 _let_24)))) (and (and (and (and (and (and (and (and (and (and (and (and (and (not (= _let_0 _let_1)) (not (= _let_0 _let_2))) (not (= _let_0 _let_3))) (not (= _let_0 _let_4))) (not (= _let_1 _let_2))) (not (= _let_1 _let_3))) (not (= _let_1 _let_4))) (not (= _let_2 _let_3))) (not (= _let_2 _let_4))) (not (= _let_3 _let_4))) (and (and (and (and (and (and (and (and (and (not (= _let_5 _let_6)) (not (= _let_5 _let_7))) (not (= _let_5 _let_8))) (not (= _let_5 _let_9))) (not (= _let_6 _let_7))) (not (= _let_6 _let_8))) (not (= _let_6 _let_9))) (not (= _let_7 _let_8))) (not (= _let_7 _let_9))) (not (= _let_8 _let_9)))) (and (and (and (and (and (and (and (and (and (not (= _let_10 _let_11)) (not (= _let_10 _let_12))) (not (= _let_10 _let_13))) (not (= _let_10 _let_14))) (not (= _let_11 _let_12))) (not (= _let_11 _let_13))) (not (= _let_11 _let_14))) (not (= _let_12 _let_13))) (not (= _let_12 _let_14))) (not (= _let_13 _let_14)))) (and (and (and (and (and (and (and (and (and (not (= _let_15 _let_16)) (not (= _let_15 _let_17))) (not (= _let_15 _let_18))) (not (= _let_15 _let_19))) (not (= _let_16 _let_17))) (not (= _let_16 _let_18))) (not (= _let_16 _let_19))) (not (= _let_17 _let_18))) (not (= _let_17 _let_19))) (not (= _let_18 _let_19)))) (and (and (and (and (and (and (and (and (and (not (= _let_20 _let_21)) (not (= _let_20 _let_22))) (not (= _let_20 _let_23))) (not (= _let_20 _let_24))) (not (= _let_21 _let_22))) (not (= _let_21 _let_23))) (not (= _let_21 _let_24))) (not (= _let_22 _let_23))) (not (= _let_22 _let_24))) (not (= _let_23 _let_24)))))))))))))))))))))))))))))))
(assert (and (and (and (and (and (and (and (and (and (not (= e0 e1)) (not (= e0 e2))) (not (= e0 e3))) (not (= e0 e4))) (not (= e1 e2))) (not (= e1 e3))) (not (= e1 e4))) (not (= e2 e3))) (not (= e2 e4))) (not (= e3 e4))))
(check-sat-assuming ( (not false) ))
