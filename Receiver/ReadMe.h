/* 
 *  Overview
 *  
 *   This is the new paradyme where the callback is lean, just setting up
 *    a queue of items for loop to take care of.  This is an attempt to be
 *    sure that there is no re-entrancy problem with the callback.  It just
 *    plots the raw data into its proper slow by MAC and the routine in loop
 *    check for the ready flag and processes it as it comes to it.  No data
 *    going into the wrong hole!  And we know how painful that can be!!!
 *  
 *  Data formating
 *  
 *   The incoming packet has the temperature and humidity readings
 *    multiplied by 100 and converted to an integer.  It is kept this
 *    way when moved to the slots pigeonholes.  It must be converted 
 *    to a float when printing or displaying and using these values for 
 *    computation and display.
 *  
 *  Temperature Conversion
 * 
 *   I did a sneaky on the temperature conversion.  I call a routine 
 *   (convertTemp) every time I output a temperature to the display.  
 *   This routine is the only
 *    place the conversion to F is known about.  If conversion to F is requested
 *    via the define, then it converts and returns it, else it just bounces back
 *    what was passed to it.  Yes, a tiny bit of overhead but it means that I don't
 *    have to do a #define for every temperature.  Yes, I do have to code a call to
 *    this routine but that is less complex and distracting from the code than a 
 *    bunch of #ifdef/#else/#endif's coded.
 *  
*/
