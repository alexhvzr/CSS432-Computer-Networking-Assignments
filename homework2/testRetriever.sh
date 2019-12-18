./Retriever www.google.com /index.html 80
cat requestResponse.txt

./Retriever https://www.facebook.com /test.html 80
cat requestResponse.txt

./Retriever http://www.streamofbytes.net /index.html 80
cat requestResponse.txt

./Retriever www.alexhvzr.com /index.html 80 # Error
cat requestResponse.txt

./Retriever www.linkedin.com /index.html 80 # Error
cat requestResponse.txt

# ./BadRetriever $1 /testFile.txt $2 # Access using a bad request
#./Retriever ww.wertwe.com /index.html $2 # Access an unauthorized file
#cat requestResponse.txt

#./Retriever $1 ../test.txt $2 # Access a forbidden file
#cat requestResponse.txt

#./Retriever $1 test.txt $2 # Access a file that does not exist
#cat requestResponse.txt