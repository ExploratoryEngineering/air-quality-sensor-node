pandoc --pdf-engine=xelatex \
	   --from markdown \
       --template eisvogel \
	   -o aqs.pdf \
	   --toc \
	   --listings \
	     00-index.md \
         01-introduction.md \
         02-build.md \
		 03-use.md \
         04-sensors.md 



