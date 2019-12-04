;; Copyright (C) 2019 Amar M. Singh

;; This program is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <https://www.gnu.org/licenses/>.

(use-modules (guix packages)
             (guix build download)
             (guix download)
             (guix git-download)
             (guix build-system cmake)
             (guix build-system gnu)
             ((guix licenses) #:prefix license:)
             (gnu packages gl)
             (gnu packages curl)
             (gnu packages cmake)
             (gnu packages sdl)
             (gnu packages audio)
             (gnu packages freedesktop)
             (gnu packages fontutils)
             (gnu packages xiph)
             (gnu packages imagemagick)
             (gnu packages compression)
             (gnu packages wget))

(define-public openspades-resources-nonfree
  (package
    (name "openspades-resources-nonfree")
    (version "1")
    (source (origin
              (method url-fetch)
              (uri (string-append "https://github.com/yvt/openspades-paks/releases/"
                                  "download/r33/OpenSpadesDevPackage-r33.zip"))
              (sha256
               (base32
                "1bd2fyn7mlxa3xnsvzj08xjzw02baimqvmnix07blfhb78rdq9q9"))))
    (build-system gnu-build-system)
    (native-inputs
     `(("unzip" ,unzip)))
    (arguments
     `(#:modules ((guix build utils)
                  (ice-9 ftw)
                  (guix build gnu-build-system))
       #:phases (modify-phases %standard-phases
                  (delete 'configure)
                  (delete 'build)
                  (delete 'check)
                  (replace 'install
                    (lambda* _
                      (let ((res-dir (string-append %output "/resources")))
                        (delete-file "../environment-variables")
                        (copy-recursively "../" res-dir)
                        #t))))))
    (home-page "https://openspades.yvt.jp/")
    (synopsis "Nonfree resources for OpenSpades")
    (description "This package provides following paks:

- `pak000-Nonfree.pak` contains essential game assets which are
incompatible with GPLv3. Please do not redistribute it separately from
OpenSpades releases or compiled binaries.

- `font-unifont.pak` contains a font that covers all 65536 characters
of Unicode BMP. Licensed under GPLv2.
")
    (license (list license:gpl2
                   (license:fsf-free "https://raw.githubusercontent.com/yvt/openspades-paks/master/Nonfree/LICENSE.md"
                                    "OpenSpades Non-GPL Pak License, limited redistribution no modification")))))

(define-public openspades
  (package
    (name "openspades")
    (version "0.1.3")
    (source (origin
              (method url-fetch)
              (uri (string-append "https://github.com/yvt/openspades/archive/v"
                                  version ".tar.gz"))
              (sha256
               (base32
                "18xy69saip8ddbjx51kfq711s8l0wgwpppgch7ci41zqd3ssmmzc"))
              (patches '("guix/guix-pkg-1.patch"))))
    (build-system cmake-build-system)
    (inputs
     `(("glew" ,glew)
       ("curl" ,curl)
       ("sdl2-image" ,sdl2-image)
       ("freealut" ,freealut)
       ("xdg-utils" ,xdg-utils)
       ("freetype" ,freetype)
       ("opus" ,opus)
       ("opusfile" ,opusfile)
       ("openal" ,openal)
       ("imagemagick" ,imagemagick)))
    (native-inputs
     `(("openspades-resources-nonfree" ,openspades-resources-nonfree)
       ("cmake" ,cmake)))
    (arguments
     `(#:modules ((guix build utils)
                  (ice-9 ftw)
                  (guix build gnu-build-system)
                  (guix build cmake-build-system))
       #:phases (modify-phases %standard-phases
                  (delete 'configure)
                  (add-before 'build 'openspades-cmake
                    (lambda* (#:key inputs outputs #:allow-other-keys)
                      (let ()
                        (display (scandir "."))
                        (mkdir "build")
                        (chdir "build")
                        (system "cmake ..")
                        (let ((nonfree-res
                               (string-append (assoc-ref inputs
                                                         "openspades-resources-nonfree"))))
                          (display nonfree-res)
                          (copy-file (string-append nonfree-res
                                                           "/resources/Nonfree/pak000-Nonfree.pak")
                                            "./pak000-Nonfree.pak")
                          (copy-file (string-append nonfree-res
                                                           "/resources/OfficialMods/font-unifont.pak")
                                            "./font-unifont.pak"))
                        #t)))
                  (delete 'check))))
    (home-page "https://openspades.yvt.jp/")
    (synopsis "Open-Source Voxel First Person Shooter")
    (description "OpenSpades is a compatible client of Ace of Spades 0.75.")
    (license license:gpl3)))

openspades
