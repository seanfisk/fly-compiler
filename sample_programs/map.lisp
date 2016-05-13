(defun is-positive (x)
  (if (> x 0) 1 0))

(print-list
 (map is-positive
      (list -20 46 -77 -100 0 51 1)))
