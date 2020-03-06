(set-option :incremental false)
(set-info :source "Specification and verification of a 8N1 decoder.
Geoffrey Brown, Indiana University <geobrown@cs.indiana.edu>
Lee Pike, Galois Connections, Inc. <leepike@galois.com> 

Translated into CVC format by Leonardo de Moura.

This benchmark was automatically translated into SMT-LIB format from
CVC format using CVC Lite")
(set-info :status unsat)
(set-info :category "industrial")
(set-info :difficulty "0")
(set-logic QF_LRA)
(declare-fun x_0 () Real)
(declare-fun x_1 () Real)
(declare-fun x_2 () Real)
(declare-fun x_3 () Real)
(declare-fun x_4 () Bool)
(declare-fun x_5 () Real)
(declare-fun x_6 () Real)
(declare-fun x_7 () Real)
(declare-fun x_8 () Real)
(declare-fun x_9 () Real)
(declare-fun x_10 () Bool)
(declare-fun x_11 () Real)
(declare-fun x_12 () Bool)
(declare-fun x_13 () Real)
(declare-fun x_14 () Bool)
(declare-fun x_15 () Real)
(declare-fun x_16 () Bool)
(declare-fun x_17 () Real)
(declare-fun x_18 () Bool)
(declare-fun x_19 () Real)
(declare-fun x_20 () Bool)
(declare-fun x_21 () Real)
(declare-fun x_22 () Bool)
(declare-fun x_23 () Real)
(declare-fun x_24 () Bool)
(declare-fun x_25 () Real)
(declare-fun x_26 () Real)
(declare-fun x_27 () Real)
(declare-fun x_28 () Real)
(declare-fun x_29 () Real)
(declare-fun x_30 () Real)
(declare-fun x_31 () Real)
(declare-fun x_32 () Real)
(declare-fun x_33 () Real)
(declare-fun x_34 () Real)
(declare-fun x_35 () Real)
(declare-fun x_36 () Real)
(declare-fun x_37 () Real)
(declare-fun x_38 () Real)
(declare-fun x_39 () Real)
(declare-fun x_40 () Real)
(declare-fun x_41 () Real)
(declare-fun x_42 () Real)
(declare-fun x_43 () Real)
(declare-fun x_44 () Real)
(declare-fun x_45 () Real)
(declare-fun x_46 () Real)
(declare-fun x_47 () Real)
(declare-fun x_48 () Real)
(declare-fun x_49 () Real)
(declare-fun x_50 () Real)
(declare-fun x_51 () Real)
(declare-fun x_52 () Real)
(declare-fun x_53 () Real)
(declare-fun x_54 () Real)
(declare-fun x_55 () Real)
(declare-fun x_56 () Real)
(declare-fun x_57 () Real)
(declare-fun x_58 () Real)
(declare-fun x_59 () Real)
(declare-fun x_60 () Real)
(declare-fun x_61 () Real)
(declare-fun x_62 () Real)
(declare-fun x_63 () Real)
(declare-fun x_64 () Real)
(declare-fun x_65 () Real)
(declare-fun x_66 () Real)
(declare-fun x_67 () Real)
(declare-fun x_68 () Real)
(declare-fun x_69 () Real)
(declare-fun x_70 () Real)
(declare-fun x_71 () Real)
(declare-fun x_72 () Real)
(declare-fun x_73 () Real)
(declare-fun x_74 () Real)
(declare-fun x_75 () Real)
(declare-fun x_76 () Real)
(declare-fun x_77 () Real)
(declare-fun x_78 () Real)
(declare-fun x_79 () Real)
(declare-fun x_80 () Real)
(declare-fun x_81 () Real)
(declare-fun x_82 () Real)
(declare-fun x_83 () Real)
(declare-fun x_84 () Real)
(declare-fun x_85 () Real)
(declare-fun x_86 () Real)
(declare-fun x_87 () Real)
(declare-fun x_88 () Real)
(declare-fun x_89 () Real)
(declare-fun x_90 () Real)
(declare-fun x_91 () Real)
(declare-fun x_92 () Real)
(declare-fun x_93 () Real)
(declare-fun x_94 () Real)
(declare-fun x_95 () Real)
(declare-fun x_96 () Real)
(declare-fun x_97 () Real)
(declare-fun x_98 () Real)
(declare-fun x_99 () Real)
(declare-fun x_100 () Real)
(declare-fun x_101 () Real)
(declare-fun x_102 () Real)
(declare-fun x_103 () Real)
(declare-fun x_104 () Real)
(declare-fun x_105 () Real)
(declare-fun x_106 () Real)
(declare-fun x_107 () Real)
(declare-fun x_108 () Real)
(declare-fun x_109 () Real)
(declare-fun x_110 () Real)
(declare-fun x_111 () Real)
(declare-fun x_112 () Real)
(declare-fun x_113 () Real)
(declare-fun x_114 () Real)
(declare-fun x_115 () Real)
(declare-fun x_116 () Real)
(declare-fun x_117 () Real)
(declare-fun x_118 () Real)
(declare-fun x_119 () Real)
(declare-fun x_120 () Real)
(declare-fun x_121 () Real)
(declare-fun x_122 () Real)
(declare-fun x_123 () Real)
(declare-fun x_124 () Real)
(declare-fun x_125 () Real)
(declare-fun x_126 () Real)
(declare-fun x_127 () Real)
(declare-fun x_128 () Real)
(declare-fun x_129 () Real)
(declare-fun x_130 () Real)
(declare-fun x_131 () Real)
(declare-fun x_132 () Real)
(declare-fun x_133 () Real)
(declare-fun x_134 () Real)
(declare-fun x_135 () Real)
(declare-fun x_136 () Real)
(declare-fun x_137 () Real)
(declare-fun x_138 () Real)
(declare-fun x_139 () Real)
(declare-fun x_140 () Real)
(declare-fun x_141 () Real)
(declare-fun x_142 () Real)
(declare-fun x_143 () Real)
(declare-fun x_144 () Real)
(declare-fun x_145 () Real)
(declare-fun x_146 () Real)
(declare-fun x_147 () Real)
(check-sat-assuming ( (let ((_let_0 (and (not x_22) (< x_23 8.0)))) (let ((_let_1 (and (not x_20) (< x_21 8.0)))) (let ((_let_2 (and (not x_18) (< x_19 8.0)))) (let ((_let_3 (and (not x_16) (< x_17 8.0)))) (let ((_let_4 (and (not x_14) (< x_15 8.0)))) (let ((_let_5 (and (not x_12) (< x_13 8.0)))) (let ((_let_6 (and (not x_10) (< x_11 8.0)))) (let ((_let_7 (and (not x_4) (< x_8 8.0)))) (let ((_let_8 (= x_1 10.0))) (let ((_let_9 (= x_26 1.0))) (let ((_let_10 (= x_10 x_4))) (let ((_let_11 (= x_27 x_5))) (let ((_let_12 (= x_28 x_29))) (let ((_let_13 (= x_30 x_7))) (let ((_let_14 (= x_8 9.0))) (let ((_let_15 (= x_28 1.0))) (let ((_let_16 (not (= x_7 1.0)))) (let ((_let_17 (not (= x_7 0.0)))) (let ((_let_18 (= x_7 3.0))) (let ((_let_19 (= x_7 2.0))) (let ((_let_20 (= x_11 x_8))) (let ((_let_21 (= x_32 x_33))) (let ((_let_22 (= x_26 x_0))) (let ((_let_23 (= x_34 x_1))) (let ((_let_24 (= x_35 x_2))) (let ((_let_25 (< x_1 9.0))) (let ((_let_26 (= x_9 2.0))) (let ((_let_27 (= x_8 x_1))) (let ((_let_28 (not (= x_33 x_29)))) (let ((_let_29 (= x_36 1.0))) (let ((_let_30 (= x_34 10.0))) (let ((_let_31 (= x_43 1.0))) (let ((_let_32 (= x_12 x_10))) (let ((_let_33 (= x_44 x_27))) (let ((_let_34 (= x_31 x_27))) (let ((_let_35 (= x_45 x_28))) (let ((_let_36 (= x_46 x_30))) (let ((_let_37 (= x_11 9.0))) (let ((_let_38 (= x_45 1.0))) (let ((_let_39 (not (= x_30 1.0)))) (let ((_let_40 (not (= x_30 0.0)))) (let ((_let_41 (= x_30 3.0))) (let ((_let_42 (= x_30 2.0))) (let ((_let_43 (= x_13 x_11))) (let ((_let_44 (= x_47 x_31))) (let ((_let_45 (= x_48 x_32))) (let ((_let_46 (= x_43 x_26))) (let ((_let_47 (= x_49 x_34))) (let ((_let_48 (= x_50 x_35))) (let ((_let_49 (< x_34 9.0))) (let ((_let_50 (= x_36 2.0))) (let ((_let_51 (= x_31 x_35))) (let ((_let_52 (not (= x_27 x_31)))) (let ((_let_53 (= x_11 x_34))) (let ((_let_54 (not (= x_32 x_28)))) (let ((_let_55 (= x_51 1.0))) (let ((_let_56 (= x_49 10.0))) (let ((_let_57 (= x_58 1.0))) (let ((_let_58 (= x_14 x_12))) (let ((_let_59 (= x_59 x_44))) (let ((_let_60 (= x_47 x_44))) (let ((_let_61 (= x_60 x_45))) (let ((_let_62 (= x_61 x_46))) (let ((_let_63 (= x_13 9.0))) (let ((_let_64 (= x_60 1.0))) (let ((_let_65 (not (= x_46 1.0)))) (let ((_let_66 (not (= x_46 0.0)))) (let ((_let_67 (= x_46 3.0))) (let ((_let_68 (= x_46 2.0))) (let ((_let_69 (= x_15 x_13))) (let ((_let_70 (= x_62 x_47))) (let ((_let_71 (= x_63 x_48))) (let ((_let_72 (= x_58 x_43))) (let ((_let_73 (= x_64 x_49))) (let ((_let_74 (= x_65 x_50))) (let ((_let_75 (< x_49 9.0))) (let ((_let_76 (= x_51 2.0))) (let ((_let_77 (= x_47 x_50))) (let ((_let_78 (not (= x_44 x_47)))) (let ((_let_79 (= x_13 x_49))) (let ((_let_80 (not (= x_48 x_45)))) (let ((_let_81 (= x_66 1.0))) (let ((_let_82 (= x_64 10.0))) (let ((_let_83 (= x_73 1.0))) (let ((_let_84 (= x_16 x_14))) (let ((_let_85 (= x_74 x_59))) (let ((_let_86 (= x_62 x_59))) (let ((_let_87 (= x_75 x_60))) (let ((_let_88 (= x_76 x_61))) (let ((_let_89 (= x_15 9.0))) (let ((_let_90 (= x_75 1.0))) (let ((_let_91 (not (= x_61 1.0)))) (let ((_let_92 (not (= x_61 0.0)))) (let ((_let_93 (= x_61 3.0))) (let ((_let_94 (= x_61 2.0))) (let ((_let_95 (= x_17 x_15))) (let ((_let_96 (= x_77 x_62))) (let ((_let_97 (= x_78 x_63))) (let ((_let_98 (= x_73 x_58))) (let ((_let_99 (= x_79 x_64))) (let ((_let_100 (= x_80 x_65))) (let ((_let_101 (< x_64 9.0))) (let ((_let_102 (= x_66 2.0))) (let ((_let_103 (= x_62 x_65))) (let ((_let_104 (not (= x_59 x_62)))) (let ((_let_105 (= x_15 x_64))) (let ((_let_106 (not (= x_63 x_60)))) (let ((_let_107 (= x_81 1.0))) (let ((_let_108 (= x_79 10.0))) (let ((_let_109 (= x_88 1.0))) (let ((_let_110 (= x_18 x_16))) (let ((_let_111 (= x_89 x_74))) (let ((_let_112 (= x_77 x_74))) (let ((_let_113 (= x_90 x_75))) (let ((_let_114 (= x_91 x_76))) (let ((_let_115 (= x_17 9.0))) (let ((_let_116 (= x_90 1.0))) (let ((_let_117 (not (= x_76 1.0)))) (let ((_let_118 (not (= x_76 0.0)))) (let ((_let_119 (= x_76 3.0))) (let ((_let_120 (= x_76 2.0))) (let ((_let_121 (= x_19 x_17))) (let ((_let_122 (= x_92 x_77))) (let ((_let_123 (= x_93 x_78))) (let ((_let_124 (= x_88 x_73))) (let ((_let_125 (= x_94 x_79))) (let ((_let_126 (= x_95 x_80))) (let ((_let_127 (< x_79 9.0))) (let ((_let_128 (= x_81 2.0))) (let ((_let_129 (= x_77 x_80))) (let ((_let_130 (not (= x_74 x_77)))) (let ((_let_131 (= x_17 x_79))) (let ((_let_132 (not (= x_78 x_75)))) (let ((_let_133 (= x_96 1.0))) (let ((_let_134 (= x_94 10.0))) (let ((_let_135 (= x_103 1.0))) (let ((_let_136 (= x_20 x_18))) (let ((_let_137 (= x_104 x_89))) (let ((_let_138 (= x_92 x_89))) (let ((_let_139 (= x_105 x_90))) (let ((_let_140 (= x_106 x_91))) (let ((_let_141 (= x_19 9.0))) (let ((_let_142 (= x_105 1.0))) (let ((_let_143 (not (= x_91 1.0)))) (let ((_let_144 (not (= x_91 0.0)))) (let ((_let_145 (= x_91 3.0))) (let ((_let_146 (= x_91 2.0))) (let ((_let_147 (= x_21 x_19))) (let ((_let_148 (= x_107 x_92))) (let ((_let_149 (= x_108 x_93))) (let ((_let_150 (= x_103 x_88))) (let ((_let_151 (= x_109 x_94))) (let ((_let_152 (= x_110 x_95))) (let ((_let_153 (< x_94 9.0))) (let ((_let_154 (= x_96 2.0))) (let ((_let_155 (= x_92 x_95))) (let ((_let_156 (not (= x_89 x_92)))) (let ((_let_157 (= x_19 x_94))) (let ((_let_158 (not (= x_93 x_90)))) (let ((_let_159 (= x_111 1.0))) (let ((_let_160 (= x_109 10.0))) (let ((_let_161 (= x_118 1.0))) (let ((_let_162 (= x_22 x_20))) (let ((_let_163 (= x_119 x_104))) (let ((_let_164 (= x_107 x_104))) (let ((_let_165 (= x_120 x_105))) (let ((_let_166 (= x_121 x_106))) (let ((_let_167 (= x_21 9.0))) (let ((_let_168 (= x_120 1.0))) (let ((_let_169 (not (= x_106 1.0)))) (let ((_let_170 (not (= x_106 0.0)))) (let ((_let_171 (= x_106 3.0))) (let ((_let_172 (= x_106 2.0))) (let ((_let_173 (= x_23 x_21))) (let ((_let_174 (= x_122 x_107))) (let ((_let_175 (= x_123 x_108))) (let ((_let_176 (= x_118 x_103))) (let ((_let_177 (= x_124 x_109))) (let ((_let_178 (= x_125 x_110))) (let ((_let_179 (< x_109 9.0))) (let ((_let_180 (= x_111 2.0))) (let ((_let_181 (= x_107 x_110))) (let ((_let_182 (not (= x_104 x_107)))) (let ((_let_183 (= x_21 x_109))) (let ((_let_184 (not (= x_108 x_105)))) (let ((_let_185 (= x_126 1.0))) (let ((_let_186 (= x_124 10.0))) (let ((_let_187 (= x_133 1.0))) (let ((_let_188 (= x_24 x_22))) (let ((_let_189 (= x_134 x_119))) (let ((_let_190 (= x_122 x_119))) (let ((_let_191 (= x_135 x_120))) (let ((_let_192 (= x_136 x_121))) (let ((_let_193 (= x_23 9.0))) (let ((_let_194 (= x_135 1.0))) (let ((_let_195 (not (= x_121 1.0)))) (let ((_let_196 (not (= x_121 0.0)))) (let ((_let_197 (= x_121 3.0))) (let ((_let_198 (= x_121 2.0))) (let ((_let_199 (= x_25 x_23))) (let ((_let_200 (= x_137 x_122))) (let ((_let_201 (= x_138 x_123))) (let ((_let_202 (= x_133 x_118))) (let ((_let_203 (= x_139 x_124))) (let ((_let_204 (= x_140 x_125))) (let ((_let_205 (< x_124 9.0))) (let ((_let_206 (= x_126 2.0))) (let ((_let_207 (= x_122 x_125))) (let ((_let_208 (not (= x_119 x_122)))) (let ((_let_209 (= x_23 x_124))) (let ((_let_210 (not (= x_123 x_120)))) (let ((_let_211 (= x_141 1.0))) (let ((_let_212 (not (< x_28 0.0)))) (let ((_let_213 (not (< x_45 0.0)))) (let ((_let_214 (not (< x_60 0.0)))) (let ((_let_215 (not (< x_75 0.0)))) (let ((_let_216 (not (< x_90 0.0)))) (let ((_let_217 (not (< x_105 0.0)))) (let ((_let_218 (not (< x_120 0.0)))) (let ((_let_219 (not (< x_135 0.0)))) (let ((_let_220 (= x_133 0.0))) (let ((_let_221 (= x_139 1.0))) (let ((_let_222 (= x_126 0.0))) (let ((_let_223 (= x_118 0.0))) (let ((_let_224 (= x_124 1.0))) (let ((_let_225 (= x_111 0.0))) (let ((_let_226 (= x_103 0.0))) (let ((_let_227 (= x_109 1.0))) (let ((_let_228 (= x_96 0.0))) (let ((_let_229 (= x_88 0.0))) (let ((_let_230 (= x_94 1.0))) (let ((_let_231 (= x_81 0.0))) (let ((_let_232 (= x_73 0.0))) (let ((_let_233 (= x_79 1.0))) (let ((_let_234 (= x_66 0.0))) (let ((_let_235 (= x_58 0.0))) (let ((_let_236 (= x_64 1.0))) (let ((_let_237 (= x_51 0.0))) (let ((_let_238 (= x_43 0.0))) (let ((_let_239 (= x_49 1.0))) (let ((_let_240 (= x_36 0.0))) (let ((_let_241 (= x_26 0.0))) (let ((_let_242 (= x_34 1.0))) (let ((_let_243 (= x_9 1.0))) (let ((_let_244 (not _let_205))) (let ((_let_245 (and _let_205 (not _let_207)))) (let ((_let_246 (and _let_205 _let_207))) (let ((_let_247 (and _let_246 _let_208))) (let ((_let_248 (and _let_247 _let_209))) (let ((_let_249 (not _let_179))) (let ((_let_250 (and _let_179 (not _let_181)))) (let ((_let_251 (and _let_179 _let_181))) (let ((_let_252 (and _let_251 _let_182))) (let ((_let_253 (and _let_252 _let_183))) (let ((_let_254 (not _let_153))) (let ((_let_255 (and _let_153 (not _let_155)))) (let ((_let_256 (and _let_153 _let_155))) (let ((_let_257 (and _let_256 _let_156))) (let ((_let_258 (and _let_257 _let_157))) (let ((_let_259 (not _let_127))) (let ((_let_260 (and _let_127 (not _let_129)))) (let ((_let_261 (and _let_127 _let_129))) (let ((_let_262 (and _let_261 _let_130))) (let ((_let_263 (and _let_262 _let_131))) (let ((_let_264 (not _let_101))) (let ((_let_265 (and _let_101 (not _let_103)))) (let ((_let_266 (and _let_101 _let_103))) (let ((_let_267 (and _let_266 _let_104))) (let ((_let_268 (and _let_267 _let_105))) (let ((_let_269 (not _let_75))) (let ((_let_270 (and _let_75 (not _let_77)))) (let ((_let_271 (and _let_75 _let_77))) (let ((_let_272 (and _let_271 _let_78))) (let ((_let_273 (and _let_272 _let_79))) (let ((_let_274 (not _let_49))) (let ((_let_275 (and _let_49 (not _let_51)))) (let ((_let_276 (and _let_49 _let_51))) (let ((_let_277 (and _let_276 _let_52))) (let ((_let_278 (and _let_277 _let_53))) (let ((_let_279 (= 0.0 x_2))) (let ((_let_280 (= x_31 0.0))) (let ((_let_281 (= 0.0 x_5))) (let ((_let_282 (not _let_25))) (let ((_let_283 (and _let_25 (not _let_279)))) (let ((_let_284 (and _let_25 _let_279))) (let ((_let_285 (not (= x_5 0.0)))) (let ((_let_286 (and _let_284 _let_285))) (let ((_let_287 (and _let_286 _let_27))) (let ((_let_288 (+ x_3 1.0))) (let ((_let_289 (- 16.0 x_6))) (let ((_let_290 (- 1.0 x_3))) (let ((_let_291 (* _let_290 23.0))) (let ((_let_292 (* _let_288 23.0))) (let ((_let_293 (* _let_290 16.0))) (let ((_let_294 (* _let_288 16.0))) (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (<= x_136 3.0) (>= x_136 0.0)) (<= x_133 3.0)) (>= x_133 0.0)) (<= x_121 3.0)) (>= x_121 0.0)) (<= x_118 3.0)) (>= x_118 0.0)) (<= x_106 3.0)) (>= x_106 0.0)) (<= x_103 3.0)) (>= x_103 0.0)) (<= x_91 3.0)) (>= x_91 0.0)) (<= x_88 3.0)) (>= x_88 0.0)) (<= x_76 3.0)) (>= x_76 0.0)) (<= x_73 3.0)) (>= x_73 0.0)) (<= x_61 3.0)) (>= x_61 0.0)) (<= x_58 3.0)) (>= x_58 0.0)) (<= x_46 3.0)) (>= x_46 0.0)) (<= x_43 3.0)) (>= x_43 0.0)) (<= x_30 3.0)) (>= x_30 0.0)) (<= x_26 3.0)) (>= x_26 0.0)) (<= x_7 3.0)) (>= x_7 0.0)) (<= x_0 3.0)) (>= x_0 0.0)) (not (< x_1 1.0))) (<= x_1 10.0)) (>= x_3 0.0)) (< x_3 (/ 3 151))) (>= x_6 0.0)) (< x_6 4.0)) (not (< x_8 0.0))) (<= x_8 9.0)) (not (< x_9 0.0))) (<= x_9 2.0)) (not (< x_11 0.0))) (<= x_11 9.0)) (not (< x_13 0.0))) (<= x_13 9.0)) (not (< x_15 0.0))) (<= x_15 9.0)) (not (< x_17 0.0))) (<= x_17 9.0)) (not (< x_19 0.0))) (<= x_19 9.0)) (not (< x_21 0.0))) (<= x_21 9.0)) (not (< x_23 0.0))) (<= x_23 9.0)) (not (< x_25 0.0))) (<= x_25 9.0)) _let_212) (<= x_28 1.0)) (not (< x_29 0.0))) (<= x_29 1.0)) (not (< x_32 0.0))) (<= x_32 1.0)) (not (< x_33 0.0))) (<= x_33 1.0)) (not (< x_34 1.0))) (<= x_34 10.0)) (not (< x_36 0.0))) (<= x_36 2.0)) _let_213) (<= x_45 1.0)) (not (< x_48 0.0))) (<= x_48 1.0)) (not (< x_49 1.0))) (<= x_49 10.0)) (not (< x_51 0.0))) (<= x_51 2.0)) _let_214) (<= x_60 1.0)) (not (< x_63 0.0))) (<= x_63 1.0)) (not (< x_64 1.0))) (<= x_64 10.0)) (not (< x_66 0.0))) (<= x_66 2.0)) _let_215) (<= x_75 1.0)) (not (< x_78 0.0))) (<= x_78 1.0)) (not (< x_79 1.0))) (<= x_79 10.0)) (not (< x_81 0.0))) (<= x_81 2.0)) _let_216) (<= x_90 1.0)) (not (< x_93 0.0))) (<= x_93 1.0)) (not (< x_94 1.0))) (<= x_94 10.0)) (not (< x_96 0.0))) (<= x_96 2.0)) _let_217) (<= x_105 1.0)) (not (< x_108 0.0))) (<= x_108 1.0)) (not (< x_109 1.0))) (<= x_109 10.0)) (not (< x_111 0.0))) (<= x_111 2.0)) _let_218) (<= x_120 1.0)) (not (< x_123 0.0))) (<= x_123 1.0)) (not (< x_124 1.0))) (<= x_124 10.0)) (not (< x_126 0.0))) (<= x_126 2.0)) _let_219) (<= x_135 1.0)) (not (< x_138 0.0))) (<= x_138 1.0)) (not (< x_139 1.0))) (<= x_139 10.0)) (not (< x_141 0.0))) (<= x_141 2.0)) (= x_0 1.0)) _let_8) (>= x_2 0.0)) (< x_2 _let_288)) (not x_4)) (>= x_5 0.0)) (<= x_5 _let_289)) (= x_7 1.0)) _let_14) _let_26) (or (or (and (and (and (and (and (and (and (and (and (and (= x_142 0.0) (or (and (and (and (= x_143 0.0) (< x_122 x_125)) (<= x_125 x_119)) (= x_137 x_125)) (and (and (and (= x_143 1.0) (< x_122 x_119)) (<= x_119 x_125)) (= x_137 x_119)))) _let_201) _let_202) _let_203) _let_204) _let_188) _let_189) _let_191) _let_199) _let_192) (and (and (and (and (and (and (and (and (and (= x_142 1.0) (or (or (and (and (and (and (and (= x_144 0.0) _let_186) _let_196) _let_187) _let_201) _let_203) (and (and (and (and (and (= x_144 1.0) _let_186) _let_195) _let_220) _let_221) _let_201)) (and (and (and (and (= x_144 2.0) (not _let_186)) (ite (or _let_198 _let_197) (or _let_220 _let_187) (= x_133 x_121))) (= x_139 (+ x_124 1.0))) (= x_138 (/ (ite _let_187 1 0) 1))))) _let_207) (ite (= x_139 10.0) (and (<= (+ x_122 _let_290) x_140) (<= x_140 (+ (+ x_122 x_3) 1.0))) (ite _let_221 (and (<= (+ x_122 _let_291) x_140) (<= x_140 (+ x_122 _let_292))) (and (<= (+ x_122 _let_293) x_140) (<= x_140 (+ x_122 _let_294)))))) _let_200) _let_188) _let_189) _let_191) _let_199) _let_192)) (and (and (and (and (and (and (and (and (= x_142 2.0) (or (and (and (and (and (= x_145 0.0) _let_190) (not x_22)) (= x_134 (+ x_122 x_6))) x_24) (and (and (and (and (= x_145 1.0) _let_190) x_22) (= x_134 (+ x_122 _let_289))) (not x_24)))) (or (and (and (and (= x_146 0.0) _let_0) (or (= x_135 0.0) _let_194)) _let_219) (and (and (= x_146 1.0) (not _let_0)) _let_191))) (or (or (or (and (and (and (and (= x_147 0.0) (not x_22)) _let_193) (= x_25 9.0)) _let_192) (and (and (and (and (= x_147 1.0) (not x_22)) _let_193) (= x_136 2.0)) (= x_25 0.0))) (and (and (and (and (= x_147 2.0) (not x_22)) (< x_23 9.0)) (= x_136 (ite (or _let_194 (= x_23 8.0)) (ite _let_195 3 x_121) (ite _let_196 2 x_121)))) (= x_25 (+ x_23 1.0)))) (and (and (and (= x_147 3.0) x_22) (= x_136 (ite _let_197 1 (ite _let_198 0 x_121)))) _let_199))) _let_200) _let_201) _let_202) _let_203) _let_204))) (or (or (or (or (and (and _let_206 (or (or _let_244 _let_245) _let_248)) (= x_141 2.0)) (and (and _let_206 (or (and _let_247 (not _let_209)) (and _let_246 (= x_119 x_122)))) _let_211)) (and (and (and (and (and _let_206 _let_205) _let_207) _let_208) _let_209) (= x_141 0.0))) (and _let_185 _let_211)) (and (and _let_222 (or (or (and _let_244 _let_210) (and _let_245 _let_210)) (and _let_248 _let_210))) _let_211))) (or (or (and (and (and (and (and (and (and (and (and (and (= x_127 0.0) (or (and (and (and (= x_128 0.0) (< x_107 x_110)) (<= x_110 x_104)) (= x_122 x_110)) (and (and (and (= x_128 1.0) (< x_107 x_104)) (<= x_104 x_110)) (= x_122 x_104)))) _let_175) _let_176) _let_177) _let_178) _let_162) _let_163) _let_165) _let_173) _let_166) (and (and (and (and (and (and (and (and (and (= x_127 1.0) (or (or (and (and (and (and (and (= x_129 0.0) _let_160) _let_170) _let_161) _let_175) _let_177) (and (and (and (and (and (= x_129 1.0) _let_160) _let_169) _let_223) _let_224) _let_175)) (and (and (and (and (= x_129 2.0) (not _let_160)) (ite (or _let_172 _let_171) (or _let_223 _let_161) (= x_118 x_106))) (= x_124 (+ x_109 1.0))) (= x_123 (/ (ite _let_161 1 0) 1))))) _let_181) (ite _let_186 (and (<= (+ x_107 _let_290) x_125) (<= x_125 (+ (+ x_107 x_3) 1.0))) (ite _let_224 (and (<= (+ x_107 _let_291) x_125) (<= x_125 (+ x_107 _let_292))) (and (<= (+ x_107 _let_293) x_125) (<= x_125 (+ x_107 _let_294)))))) _let_174) _let_162) _let_163) _let_165) _let_173) _let_166)) (and (and (and (and (and (and (and (and (= x_127 2.0) (or (and (and (and (and (= x_130 0.0) _let_164) (not x_20)) (= x_119 (+ x_107 x_6))) x_22) (and (and (and (and (= x_130 1.0) _let_164) x_20) (= x_119 (+ x_107 _let_289))) (not x_22)))) (or (and (and (and (= x_131 0.0) _let_1) (or (= x_120 0.0) _let_168)) _let_218) (and (and (= x_131 1.0) (not _let_1)) _let_165))) (or (or (or (and (and (and (and (= x_132 0.0) (not x_20)) _let_167) _let_193) _let_166) (and (and (and (and (= x_132 1.0) (not x_20)) _let_167) _let_198) (= x_23 0.0))) (and (and (and (and (= x_132 2.0) (not x_20)) (< x_21 9.0)) (= x_121 (ite (or _let_168 (= x_21 8.0)) (ite _let_169 3 x_106) (ite _let_170 2 x_106)))) (= x_23 (+ x_21 1.0)))) (and (and (and (= x_132 3.0) x_20) (= x_121 (ite _let_171 1 (ite _let_172 0 x_106)))) _let_173))) _let_174) _let_175) _let_176) _let_177) _let_178))) (or (or (or (or (and (and _let_180 (or (or _let_249 _let_250) _let_253)) _let_206) (and (and _let_180 (or (and _let_252 (not _let_183)) (and _let_251 (= x_104 x_107)))) _let_185)) (and (and (and (and (and _let_180 _let_179) _let_181) _let_182) _let_183) _let_222)) (and _let_159 _let_185)) (and (and _let_225 (or (or (and _let_249 _let_184) (and _let_250 _let_184)) (and _let_253 _let_184))) _let_185))) (or (or (and (and (and (and (and (and (and (and (and (and (= x_112 0.0) (or (and (and (and (= x_113 0.0) (< x_92 x_95)) (<= x_95 x_89)) (= x_107 x_95)) (and (and (and (= x_113 1.0) (< x_92 x_89)) (<= x_89 x_95)) (= x_107 x_89)))) _let_149) _let_150) _let_151) _let_152) _let_136) _let_137) _let_139) _let_147) _let_140) (and (and (and (and (and (and (and (and (and (= x_112 1.0) (or (or (and (and (and (and (and (= x_114 0.0) _let_134) _let_144) _let_135) _let_149) _let_151) (and (and (and (and (and (= x_114 1.0) _let_134) _let_143) _let_226) _let_227) _let_149)) (and (and (and (and (= x_114 2.0) (not _let_134)) (ite (or _let_146 _let_145) (or _let_226 _let_135) (= x_103 x_91))) (= x_109 (+ x_94 1.0))) (= x_108 (/ (ite _let_135 1 0) 1))))) _let_155) (ite _let_160 (and (<= (+ x_92 _let_290) x_110) (<= x_110 (+ (+ x_92 x_3) 1.0))) (ite _let_227 (and (<= (+ x_92 _let_291) x_110) (<= x_110 (+ x_92 _let_292))) (and (<= (+ x_92 _let_293) x_110) (<= x_110 (+ x_92 _let_294)))))) _let_148) _let_136) _let_137) _let_139) _let_147) _let_140)) (and (and (and (and (and (and (and (and (= x_112 2.0) (or (and (and (and (and (= x_115 0.0) _let_138) (not x_18)) (= x_104 (+ x_92 x_6))) x_20) (and (and (and (and (= x_115 1.0) _let_138) x_18) (= x_104 (+ x_92 _let_289))) (not x_20)))) (or (and (and (and (= x_116 0.0) _let_2) (or (= x_105 0.0) _let_142)) _let_217) (and (and (= x_116 1.0) (not _let_2)) _let_139))) (or (or (or (and (and (and (and (= x_117 0.0) (not x_18)) _let_141) _let_167) _let_140) (and (and (and (and (= x_117 1.0) (not x_18)) _let_141) _let_172) (= x_21 0.0))) (and (and (and (and (= x_117 2.0) (not x_18)) (< x_19 9.0)) (= x_106 (ite (or _let_142 (= x_19 8.0)) (ite _let_143 3 x_91) (ite _let_144 2 x_91)))) (= x_21 (+ x_19 1.0)))) (and (and (and (= x_117 3.0) x_18) (= x_106 (ite _let_145 1 (ite _let_146 0 x_91)))) _let_147))) _let_148) _let_149) _let_150) _let_151) _let_152))) (or (or (or (or (and (and _let_154 (or (or _let_254 _let_255) _let_258)) _let_180) (and (and _let_154 (or (and _let_257 (not _let_157)) (and _let_256 (= x_89 x_92)))) _let_159)) (and (and (and (and (and _let_154 _let_153) _let_155) _let_156) _let_157) _let_225)) (and _let_133 _let_159)) (and (and _let_228 (or (or (and _let_254 _let_158) (and _let_255 _let_158)) (and _let_258 _let_158))) _let_159))) (or (or (and (and (and (and (and (and (and (and (and (and (= x_97 0.0) (or (and (and (and (= x_98 0.0) (< x_77 x_80)) (<= x_80 x_74)) (= x_92 x_80)) (and (and (and (= x_98 1.0) (< x_77 x_74)) (<= x_74 x_80)) (= x_92 x_74)))) _let_123) _let_124) _let_125) _let_126) _let_110) _let_111) _let_113) _let_121) _let_114) (and (and (and (and (and (and (and (and (and (= x_97 1.0) (or (or (and (and (and (and (and (= x_99 0.0) _let_108) _let_118) _let_109) _let_123) _let_125) (and (and (and (and (and (= x_99 1.0) _let_108) _let_117) _let_229) _let_230) _let_123)) (and (and (and (and (= x_99 2.0) (not _let_108)) (ite (or _let_120 _let_119) (or _let_229 _let_109) (= x_88 x_76))) (= x_94 (+ x_79 1.0))) (= x_93 (/ (ite _let_109 1 0) 1))))) _let_129) (ite _let_134 (and (<= (+ x_77 _let_290) x_95) (<= x_95 (+ (+ x_77 x_3) 1.0))) (ite _let_230 (and (<= (+ x_77 _let_291) x_95) (<= x_95 (+ x_77 _let_292))) (and (<= (+ x_77 _let_293) x_95) (<= x_95 (+ x_77 _let_294)))))) _let_122) _let_110) _let_111) _let_113) _let_121) _let_114)) (and (and (and (and (and (and (and (and (= x_97 2.0) (or (and (and (and (and (= x_100 0.0) _let_112) (not x_16)) (= x_89 (+ x_77 x_6))) x_18) (and (and (and (and (= x_100 1.0) _let_112) x_16) (= x_89 (+ x_77 _let_289))) (not x_18)))) (or (and (and (and (= x_101 0.0) _let_3) (or (= x_90 0.0) _let_116)) _let_216) (and (and (= x_101 1.0) (not _let_3)) _let_113))) (or (or (or (and (and (and (and (= x_102 0.0) (not x_16)) _let_115) _let_141) _let_114) (and (and (and (and (= x_102 1.0) (not x_16)) _let_115) _let_146) (= x_19 0.0))) (and (and (and (and (= x_102 2.0) (not x_16)) (< x_17 9.0)) (= x_91 (ite (or _let_116 (= x_17 8.0)) (ite _let_117 3 x_76) (ite _let_118 2 x_76)))) (= x_19 (+ x_17 1.0)))) (and (and (and (= x_102 3.0) x_16) (= x_91 (ite _let_119 1 (ite _let_120 0 x_76)))) _let_121))) _let_122) _let_123) _let_124) _let_125) _let_126))) (or (or (or (or (and (and _let_128 (or (or _let_259 _let_260) _let_263)) _let_154) (and (and _let_128 (or (and _let_262 (not _let_131)) (and _let_261 (= x_74 x_77)))) _let_133)) (and (and (and (and (and _let_128 _let_127) _let_129) _let_130) _let_131) _let_228)) (and _let_107 _let_133)) (and (and _let_231 (or (or (and _let_259 _let_132) (and _let_260 _let_132)) (and _let_263 _let_132))) _let_133))) (or (or (and (and (and (and (and (and (and (and (and (and (= x_82 0.0) (or (and (and (and (= x_83 0.0) (< x_62 x_65)) (<= x_65 x_59)) (= x_77 x_65)) (and (and (and (= x_83 1.0) (< x_62 x_59)) (<= x_59 x_65)) (= x_77 x_59)))) _let_97) _let_98) _let_99) _let_100) _let_84) _let_85) _let_87) _let_95) _let_88) (and (and (and (and (and (and (and (and (and (= x_82 1.0) (or (or (and (and (and (and (and (= x_84 0.0) _let_82) _let_92) _let_83) _let_97) _let_99) (and (and (and (and (and (= x_84 1.0) _let_82) _let_91) _let_232) _let_233) _let_97)) (and (and (and (and (= x_84 2.0) (not _let_82)) (ite (or _let_94 _let_93) (or _let_232 _let_83) (= x_73 x_61))) (= x_79 (+ x_64 1.0))) (= x_78 (/ (ite _let_83 1 0) 1))))) _let_103) (ite _let_108 (and (<= (+ x_62 _let_290) x_80) (<= x_80 (+ (+ x_62 x_3) 1.0))) (ite _let_233 (and (<= (+ x_62 _let_291) x_80) (<= x_80 (+ x_62 _let_292))) (and (<= (+ x_62 _let_293) x_80) (<= x_80 (+ x_62 _let_294)))))) _let_96) _let_84) _let_85) _let_87) _let_95) _let_88)) (and (and (and (and (and (and (and (and (= x_82 2.0) (or (and (and (and (and (= x_85 0.0) _let_86) (not x_14)) (= x_74 (+ x_62 x_6))) x_16) (and (and (and (and (= x_85 1.0) _let_86) x_14) (= x_74 (+ x_62 _let_289))) (not x_16)))) (or (and (and (and (= x_86 0.0) _let_4) (or (= x_75 0.0) _let_90)) _let_215) (and (and (= x_86 1.0) (not _let_4)) _let_87))) (or (or (or (and (and (and (and (= x_87 0.0) (not x_14)) _let_89) _let_115) _let_88) (and (and (and (and (= x_87 1.0) (not x_14)) _let_89) _let_120) (= x_17 0.0))) (and (and (and (and (= x_87 2.0) (not x_14)) (< x_15 9.0)) (= x_76 (ite (or _let_90 (= x_15 8.0)) (ite _let_91 3 x_61) (ite _let_92 2 x_61)))) (= x_17 (+ x_15 1.0)))) (and (and (and (= x_87 3.0) x_14) (= x_76 (ite _let_93 1 (ite _let_94 0 x_61)))) _let_95))) _let_96) _let_97) _let_98) _let_99) _let_100))) (or (or (or (or (and (and _let_102 (or (or _let_264 _let_265) _let_268)) _let_128) (and (and _let_102 (or (and _let_267 (not _let_105)) (and _let_266 (= x_59 x_62)))) _let_107)) (and (and (and (and (and _let_102 _let_101) _let_103) _let_104) _let_105) _let_231)) (and _let_81 _let_107)) (and (and _let_234 (or (or (and _let_264 _let_106) (and _let_265 _let_106)) (and _let_268 _let_106))) _let_107))) (or (or (and (and (and (and (and (and (and (and (and (and (= x_67 0.0) (or (and (and (and (= x_68 0.0) (< x_47 x_50)) (<= x_50 x_44)) (= x_62 x_50)) (and (and (and (= x_68 1.0) (< x_47 x_44)) (<= x_44 x_50)) (= x_62 x_44)))) _let_71) _let_72) _let_73) _let_74) _let_58) _let_59) _let_61) _let_69) _let_62) (and (and (and (and (and (and (and (and (and (= x_67 1.0) (or (or (and (and (and (and (and (= x_69 0.0) _let_56) _let_66) _let_57) _let_71) _let_73) (and (and (and (and (and (= x_69 1.0) _let_56) _let_65) _let_235) _let_236) _let_71)) (and (and (and (and (= x_69 2.0) (not _let_56)) (ite (or _let_68 _let_67) (or _let_235 _let_57) (= x_58 x_46))) (= x_64 (+ x_49 1.0))) (= x_63 (/ (ite _let_57 1 0) 1))))) _let_77) (ite _let_82 (and (<= (+ x_47 _let_290) x_65) (<= x_65 (+ (+ x_47 x_3) 1.0))) (ite _let_236 (and (<= (+ x_47 _let_291) x_65) (<= x_65 (+ x_47 _let_292))) (and (<= (+ x_47 _let_293) x_65) (<= x_65 (+ x_47 _let_294)))))) _let_70) _let_58) _let_59) _let_61) _let_69) _let_62)) (and (and (and (and (and (and (and (and (= x_67 2.0) (or (and (and (and (and (= x_70 0.0) _let_60) (not x_12)) (= x_59 (+ x_47 x_6))) x_14) (and (and (and (and (= x_70 1.0) _let_60) x_12) (= x_59 (+ x_47 _let_289))) (not x_14)))) (or (and (and (and (= x_71 0.0) _let_5) (or (= x_60 0.0) _let_64)) _let_214) (and (and (= x_71 1.0) (not _let_5)) _let_61))) (or (or (or (and (and (and (and (= x_72 0.0) (not x_12)) _let_63) _let_89) _let_62) (and (and (and (and (= x_72 1.0) (not x_12)) _let_63) _let_94) (= x_15 0.0))) (and (and (and (and (= x_72 2.0) (not x_12)) (< x_13 9.0)) (= x_61 (ite (or _let_64 (= x_13 8.0)) (ite _let_65 3 x_46) (ite _let_66 2 x_46)))) (= x_15 (+ x_13 1.0)))) (and (and (and (= x_72 3.0) x_12) (= x_61 (ite _let_67 1 (ite _let_68 0 x_46)))) _let_69))) _let_70) _let_71) _let_72) _let_73) _let_74))) (or (or (or (or (and (and _let_76 (or (or _let_269 _let_270) _let_273)) _let_102) (and (and _let_76 (or (and _let_272 (not _let_79)) (and _let_271 (= x_44 x_47)))) _let_81)) (and (and (and (and (and _let_76 _let_75) _let_77) _let_78) _let_79) _let_234)) (and _let_55 _let_81)) (and (and _let_237 (or (or (and _let_269 _let_80) (and _let_270 _let_80)) (and _let_273 _let_80))) _let_81))) (or (or (and (and (and (and (and (and (and (and (and (and (= x_52 0.0) (or (and (and (and (= x_53 0.0) (< x_31 x_35)) (<= x_35 x_27)) (= x_47 x_35)) (and (and (and (= x_53 1.0) (< x_31 x_27)) (<= x_27 x_35)) (= x_47 x_27)))) _let_45) _let_46) _let_47) _let_48) _let_32) _let_33) _let_35) _let_43) _let_36) (and (and (and (and (and (and (and (and (and (= x_52 1.0) (or (or (and (and (and (and (and (= x_54 0.0) _let_30) _let_40) _let_31) _let_45) _let_47) (and (and (and (and (and (= x_54 1.0) _let_30) _let_39) _let_238) _let_239) _let_45)) (and (and (and (and (= x_54 2.0) (not _let_30)) (ite (or _let_42 _let_41) (or _let_238 _let_31) (= x_43 x_30))) (= x_49 (+ x_34 1.0))) (= x_48 (/ (ite _let_31 1 0) 1))))) _let_51) (ite _let_56 (and (<= (+ x_31 _let_290) x_50) (<= x_50 (+ (+ x_31 x_3) 1.0))) (ite _let_239 (and (<= (+ x_31 _let_291) x_50) (<= x_50 (+ x_31 _let_292))) (and (<= (+ x_31 _let_293) x_50) (<= x_50 (+ x_31 _let_294)))))) _let_44) _let_32) _let_33) _let_35) _let_43) _let_36)) (and (and (and (and (and (and (and (and (= x_52 2.0) (or (and (and (and (and (= x_55 0.0) _let_34) (not x_10)) (= x_44 (+ x_31 x_6))) x_12) (and (and (and (and (= x_55 1.0) _let_34) x_10) (= x_44 (+ x_31 _let_289))) (not x_12)))) (or (and (and (and (= x_56 0.0) _let_6) (or (= x_45 0.0) _let_38)) _let_213) (and (and (= x_56 1.0) (not _let_6)) _let_35))) (or (or (or (and (and (and (and (= x_57 0.0) (not x_10)) _let_37) _let_63) _let_36) (and (and (and (and (= x_57 1.0) (not x_10)) _let_37) _let_68) (= x_13 0.0))) (and (and (and (and (= x_57 2.0) (not x_10)) (< x_11 9.0)) (= x_46 (ite (or _let_38 (= x_11 8.0)) (ite _let_39 3 x_30) (ite _let_40 2 x_30)))) (= x_13 (+ x_11 1.0)))) (and (and (and (= x_57 3.0) x_10) (= x_46 (ite _let_41 1 (ite _let_42 0 x_30)))) _let_43))) _let_44) _let_45) _let_46) _let_47) _let_48))) (or (or (or (or (and (and _let_50 (or (or _let_274 _let_275) _let_278)) _let_76) (and (and _let_50 (or (and _let_277 (not _let_53)) (and _let_276 (= x_27 x_31)))) _let_55)) (and (and (and (and (and _let_50 _let_49) _let_51) _let_52) _let_53) _let_237)) (and _let_29 _let_55)) (and (and _let_240 (or (or (and _let_274 _let_54) (and _let_275 _let_54)) (and _let_278 _let_54))) _let_55))) (or (or (and (and (and (and (and (and (and (and (and (and (= x_37 0.0) (or (and (and (and (= x_38 0.0) (> x_2 0.0)) (<= x_2 x_5)) (= x_31 x_2)) (and (and (and (= x_38 1.0) (> x_5 0.0)) (<= x_5 x_2)) (= x_31 x_5)))) _let_21) _let_22) _let_23) _let_24) _let_10) _let_11) _let_12) _let_20) _let_13) (and (and (and (and (and (and (and (and (and (= x_37 1.0) (or (or (and (and (and (and (and (= x_39 0.0) _let_8) _let_17) _let_9) _let_21) _let_23) (and (and (and (and (and (= x_39 1.0) _let_8) _let_16) _let_241) _let_242) _let_21)) (and (and (and (and (= x_39 2.0) (not _let_8)) (ite (or _let_19 _let_18) (or _let_241 _let_9) (= x_26 x_7))) (= x_34 (+ x_1 1.0))) (= x_32 (/ (ite _let_9 1 0) 1))))) _let_279) (ite _let_30 (and (<= (+ 0.0 _let_290) x_35) (<= x_35 (+ (+ 0.0 x_3) 1.0))) (ite _let_242 (and (<= (+ 0.0 _let_291) x_35) (<= x_35 (+ 0.0 _let_292))) (and (<= (+ 0.0 _let_293) x_35) (<= x_35 (+ 0.0 _let_294)))))) _let_280) _let_10) _let_11) _let_12) _let_20) _let_13)) (and (and (and (and (and (and (and (and (= x_37 2.0) (or (and (and (and (and (= x_40 0.0) _let_281) (not x_4)) (= x_27 (+ 0.0 x_6))) x_10) (and (and (and (and (= x_40 1.0) _let_281) x_4) (= x_27 (+ 0.0 _let_289))) (not x_10)))) (or (and (and (and (= x_41 0.0) _let_7) (or (= x_28 0.0) _let_15)) _let_212) (and (and (= x_41 1.0) (not _let_7)) _let_12))) (or (or (or (and (and (and (and (= x_42 0.0) (not x_4)) _let_14) _let_37) _let_13) (and (and (and (and (= x_42 1.0) (not x_4)) _let_14) _let_42) (= x_11 0.0))) (and (and (and (and (= x_42 2.0) (not x_4)) (< x_8 9.0)) (= x_30 (ite (or _let_15 (= x_8 8.0)) (ite _let_16 3 x_7) (ite _let_17 2 x_7)))) (= x_11 (+ x_8 1.0)))) (and (and (and (= x_42 3.0) x_4) (= x_30 (ite _let_18 1 (ite _let_19 0 x_7)))) _let_20))) _let_280) _let_21) _let_22) _let_23) _let_24))) (or (or (or (or (and (and _let_26 (or (or _let_282 _let_283) _let_287)) _let_50) (and (and _let_26 (or (and _let_286 (not _let_27)) (and _let_284 (= x_5 0.0)))) _let_29)) (and (and (and (and (and _let_26 _let_25) _let_279) _let_285) _let_27) _let_240)) (and _let_243 _let_29)) (and (and (= x_9 0.0) (or (or (and _let_282 _let_28) (and _let_283 _let_28)) (and _let_287 _let_28))) _let_29))) (or (or (or (or (or (or (or (or _let_211 _let_185) _let_159) _let_133) _let_107) _let_81) _let_55) _let_29) _let_243))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))) ))
