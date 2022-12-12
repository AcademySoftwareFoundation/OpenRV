//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
// Lisp definition
//
// (defun unfold (p f g seed &optional (tail-gen #'identity))                           
//   (if (funcall p seed) (funcall tail-gen seed)                                       
//       (cons (funcall f seed)                                                         
//             (unfold p f g (funcall g seed)))))                                       

\: unfold ('a[]; 
           (bool;'a) p, 
           ('a;'a) f, 
           ('a;'a) g, 
           'a seed, 
           ('a;'a) tail = \: ('a;'a a) { a; } )
{
    p(seed) ? tail(seed) : f(seed) + unfold(p, f, g, g(seed));
}

//
// (defun unfold-right (p f g seed &optional (tail nil))                                
//   (labels ((lp (seed list)                                                           
//              (if (funcall p seed) list                                               
//                  (lp (funcall g seed)                                                
//                      (cons (funcall f seed) list)))))                                
//     (lp seed tail)))                                                                 
//
                                                                                     
