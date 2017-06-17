(defun find-file-read-only-from-command-line ()
  "For use as command-line function call to initialize emacs by 
   bringing up a file (named by the first command-line 
   argument) read-only"
  (find-file-read-only (car(cdr command-line-args))))
