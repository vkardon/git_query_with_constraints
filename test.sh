#!/bin/bash

app ./books.txt "(Language == French OR Language == Spanish) AND BookNumber > 200"
echo ------------------------------------------------------------------
app ./books.txt "BookNumber > 300"
echo ------------------------------------------------------------------
app ./books.txt "Language == Russian OR Language == Spanish"
echo ------------------------------------------------------------------
app ./books.txt "Language == AAAA OR Language == BBBB"
echo ------------------------------------------------------------------
app ./books.txt "Genre == Detective AND (Language == Belgian OR Language == French)"
echo ------------------------------------------------------------------
app ./books.txt "Genre == Detective"
echo ------------------------------------------------------------------
app ./books.txt "Genre == \"Detective\" AND (Nationality == \" French \" OR Nationality == \"American\")"
echo ------------------------------------------------------------------
app ./books.txt "Genre == Detective AND Nationality IN (French, American)"
echo 


