(defun factorial (x)
  (if x
      (* x (factorial (- x 1)))
    1))

(defun print-factorial (x)
  (print (factorial x)))

(map print-factorial (list 2 4 6 8))
