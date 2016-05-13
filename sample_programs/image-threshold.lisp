(defun threshold (pixel-value)
  (if (< pixel-value 230)
      0
    255))

(image-from-list 29566 14321 (map threshold (list-from-image)))
