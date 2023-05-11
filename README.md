# git_query_with_constraints
Functional example that demonstrates query with constraints. 

It uses books.txt file to create objects with specified "name=value" properties, and then evaluate if object matches given constraints. 

Example: 
"(Language == French OR Language == Spanish) AND BookNumber > 200" 
Will select all books in French or Spanish with BookNumber greater then 200.

"Genre == Detective AND (Language == Belgian OR Language == French)"
Will select all Detective books in Belgian or French.

Check test.sh for more examples.
