(defun double (xyz)
  (+ xyz xyz))

(defun quadruple (a)
  (double (double a)))

(print (quadruple 20))
