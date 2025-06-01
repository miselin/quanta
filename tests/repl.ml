(defun repl ()
  (begin
    (print "lisp> ")
    (let ((input (read-line)))
      (begin
        (print (eval (read input)))
        (repl)))))

(repl)
