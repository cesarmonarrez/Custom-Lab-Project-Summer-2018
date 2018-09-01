#include <avr/io.h>
#include <avr/interrupt.h>
#include "io.h"
#include "io.c"
#include "timer.h"
#include <avr/eeprom.h>


#define START_BUTTON (~PINA & 0x02)
#define RESET_BUTTON (~PINA & 0x04)
#define SCORE_BUTTON (~PINA & 0x08)

unsigned short x = 0;
unsigned char tmpscore1, tmpscore2, tmpscore3 = 0;
unsigned char playerposition = 3;
unsigned char objectposition = 16;
unsigned char object2position, object3position, object4position, object5position, object6position = 22;
unsigned char object7position, object8position = 22;

unsigned char Score1sPlace = 0;
unsigned char Score10sPlace = 0;
unsigned char Score100sPlace = 0;


unsigned char tmp100scompare, tmp10scompare, tmp1scompare = 0;
unsigned char obstacle1gonecnt, obstacle2gonecnt, obstacle3gonecnt, obstacle4gonecnt, obstacle5gonecnt = 0;
unsigned char obstacle6gonecnt, obstacle7gonecnt, obstacle8gonecnt = 0;
unsigned char level1Flag = 1;
unsigned char level2Flag, level3Flag, level4Flag, level5Flag = 0;
unsigned char object1Flag, object2Flag, object3Flag, object4Flag, object5Flag, object6Flag, object7Flag, object8Flag = 0;
unsigned char CustomCharLeftFlag[] = {0x06, 0x0E, 0x1E, 0x1A, 0x12, 0x02, 0x02, 0x00}; // remember to not include last byte
unsigned char CustomCharRightFlag[] = {0x0C, 0x0E, 0x0F, 0x0B, 0x09, 0x08, 0x08, 0x00};
unsigned char CustomCharSadFace[] = {0x1F, 0x15, 0x1F, 0x1F,0x11, 0x0E, 0x1F, 0x1F};
unsigned char CustomCharTree[] = {0x1F, 0x1F, 0x1F, 0x0E, 0x04, 0x04, 0x0E, 0x0E};
unsigned char CustomCharPlayer[] = {0x0F, 0x0B, 0x0E, 0x19, 0x0E, 0x0E, 0x1A, 0x00};
unsigned char CustomCharEnemy[] = {0x1F, 0x0E, 0x15, 0x1F, 0x11, 0x0E, 0x1F, 0x15};
unsigned char CustomCharRock[] = {0x00, 0x04, 0x0E, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};
unsigned char CustomCharItem[] = {0x1F, 0x15, 0x15, 0x1F, 0x0E, 0x11, 0x1F, 0x00};
unsigned char StartMsg[] = "Press Start!";
unsigned char GameOverMsg[] = "Game Over!";
unsigned char scoremsg[] = "Score: ";
unsigned char highscoremsg[] = "High Score: ";
unsigned char Track[] = "________________________________";
unsigned char Waiting[] = "Waiting.";
unsigned char HitFlag = 0;
unsigned char ResetFlag = 0;
unsigned char cnt, cnt2, cnt5, cnt8 = 17;
unsigned char cnt3, cnt4, cnt6, cnt7 = 33;
unsigned char tickcnt = 0;

enum TestingOutput {Start, WaitButtonPress, Output, GameOver, DisplayScore} testoutput_state;
enum PositionTracker {PosStart, CalculatingPosition} position_state;
enum ObjectTracker {ObjStart, CalculatingObjLocation} obj_state;
enum HitDetection {HitStart, CalculatingHitDetect, Stop} hit_state;
enum Scoring {ScoreStart, CurrentlyScoring, StopScoring} score_state;

unsigned char StartFlag = 0;

void CreateCustomChar (unsigned char *image, unsigned char address) // takes an array of chars (to make pattern) and the address to
//where it should be saved
{
	unsigned char cnt;

	if(address < 8)                                 // the Hitachi HD44780 only has 8 available positions for custom chars
	{
		LCD_WriteCommand(0x40 + (address * 8));     //here, we set the CGRAM address


		cnt = 0;
		while (cnt < 8) {
			LCD_WriteData(image[cnt]);    //here, we go through the 8 char array to form the new custom character
			cnt++;
		}
	}
}


void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE))
	;
	/* Set up address and Data Registers */
	EEAR = uiAddress;
	EEDR = ucData;
	/* Write logical one to EEMPE */
	EECR |= (1<<EEMPE);
	/* Start eeprom write by setting EEPE */
	EECR |= (1<<EEPE);
}

unsigned char EEPROM_read(unsigned int uiAddress)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE))
	;
	/* Set up address register */
	EEAR = uiAddress;
	/* Start eeprom read by writing EERE */
	EECR |= (1<<EERE);
	/* Return data from Data Register */
	return EEDR;
}

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
}


void TickFct_Scoring()
{
	
	static unsigned char scorecounter = 0;
	
	switch(score_state) {
		case ScoreStart :
		if (StartFlag) score_state = CurrentlyScoring;
		else score_state = ScoreStart;
		break;
		
		case CurrentlyScoring :
		if (!StartFlag) score_state = ScoreStart;
		else if (HitFlag) score_state = StopScoring;
		else score_state = CurrentlyScoring;
		break;
		
		case StopScoring :
		if (!StartFlag) score_state = ScoreStart;
		else score_state = StopScoring;
		break;
		
		default :
		score_state = ScoreStart;
		break;
	}
	
	switch(score_state) {
		case ScoreStart :
		scorecounter = 0;
		break;
		
		case CurrentlyScoring :
		if (scorecounter >= 2) {
			Score1sPlace++;
			scorecounter = 0;
		}
		if (Score1sPlace >= 10) {
			Score10sPlace++;
			Score1sPlace = 0;
		}
		if (Score10sPlace >= 10) {
			Score100sPlace++;
			Score10sPlace = 0;
		}
		scorecounter++;
		break;
	}
}

void TickFct_TestOutput()
{
	
	static unsigned char gameovercounter = 0;
	
	switch(testoutput_state){
		case Start :
		testoutput_state = WaitButtonPress;
		object2position = object3position = object4position = object5position = object6position = 22;
		object7position = object8position = 22;
		obstacle1gonecnt = obstacle2gonecnt = obstacle3gonecnt = obstacle4gonecnt = obstacle5gonecnt = 0;
		playerposition = 3;
		obstacle6gonecnt = obstacle7gonecnt = obstacle8gonecnt = 0;
		level1Flag = 1;
		level2Flag = level3Flag = level4Flag = level5Flag = 0;
		object1Flag = object2Flag = object3Flag = object4Flag = object5Flag = object6Flag = object7Flag = object8Flag = 0;
		StartFlag = 0;
		HitFlag = 0;
		cnt = cnt2 = cnt5 = cnt8 = 17;
		cnt3 = cnt4 = cnt6 = cnt7 = 33;
		tickcnt = 0;
		gameovercounter = 0;
		Score1sPlace = Score10sPlace = Score100sPlace = 0;
		
		LCD_ClearScreen();
		LCD_Cursor(1);
		LCD_WriteData(4);
		LCD_WriteData(4);
		
		for (int i = 0; i < 12; i++){
			LCD_WriteData(StartMsg[i]);
		}
		LCD_WriteData(5);
		LCD_WriteData(5);
		break;
		
		case WaitButtonPress :
		if (SCORE_BUTTON && !RESET_BUTTON && !START_BUTTON) {
			testoutput_state = DisplayScore;
			LCD_ClearScreen();
			LCD_Cursor(1);
			for (int i = 0; i < 12; i++) {
				LCD_WriteData(highscoremsg[i]);
			}
		}
		else if (START_BUTTON && !RESET_BUTTON && !SCORE_BUTTON){
			testoutput_state = Output;
			StartFlag = 1;
			LCD_DisplayString(1, Track);
			playerposition = 3;
		}
		else{
			testoutput_state = WaitButtonPress;
		}
		break;
		
		case Output :
		if(HitFlag){
			testoutput_state = GameOver;
			tmp100scompare = EEPROM_read(0x04);
			tmp10scompare = EEPROM_read(0x05);
			tmp1scompare = EEPROM_read(0x06);
			
			if (Score100sPlace > tmp100scompare) {
				EEPROM_write(0x04, Score100sPlace);
				EEPROM_write(0x05, Score10sPlace);
				EEPROM_write(0x06, Score1sPlace);
			}
			else if (Score100sPlace == tmp100scompare) {
				if (Score10sPlace > tmp10scompare) {
					EEPROM_write(0x04, Score100sPlace);
					EEPROM_write(0x05, Score10sPlace);
					EEPROM_write(0x06, Score1sPlace);
				}
				else if (Score10sPlace == tmp10scompare) {
					if (Score1sPlace > tmp1scompare) {
						EEPROM_write(0x04, Score100sPlace);
						EEPROM_write(0x05, Score10sPlace);
						EEPROM_write(0x06, Score1sPlace);
					}
					else if ((Score1sPlace == tmp1scompare) || (Score1sPlace < tmp1scompare));
				}
				else if (Score10sPlace < tmp10scompare);
			}
			else if (Score100sPlace < tmp100scompare);
		}
		else if (RESET_BUTTON && !START_BUTTON) {
			testoutput_state = Start;
		}
		else{
			testoutput_state = Output;
		}
		break;
		
		case GameOver :
		if (RESET_BUTTON && !START_BUTTON) testoutput_state = Start;
		else testoutput_state = GameOver;
		break;
		
		case DisplayScore :
		if (SCORE_BUTTON && !RESET_BUTTON && !START_BUTTON) {
			testoutput_state = DisplayScore;
		}
		else {
			testoutput_state = WaitButtonPress;
			LCD_ClearScreen();
			LCD_Cursor(1);
			LCD_WriteData(4);
			LCD_WriteData(4);
			
			for (int i = 0; i < 12; i++){
				LCD_WriteData(StartMsg[i]);
			}
			LCD_WriteData(5);
			LCD_WriteData(5);
		}
		break;
		
		default :
		testoutput_state = Start;
		break;
	}
	
	switch(testoutput_state){
		case WaitButtonPress :
		break;
		
		case Output :
		LCD_Cursor(playerposition);
		LCD_WriteData(1);
		LCD_Cursor(1);
		LCD_WriteData('_');
		LCD_Cursor(17);
		LCD_WriteData('_');
		if (object1Flag){
			LCD_Cursor(objectposition);
			LCD_WriteData(3);
		}
		if (object2Flag){
			LCD_Cursor(object2position);
			LCD_WriteData(2);
		}
		if (object3Flag){
			LCD_Cursor(object3position);
			LCD_WriteData(3);
		}
		if (object4Flag){
			LCD_Cursor(object4position);
			LCD_WriteData(2);
		}
		if (object5Flag) {
			LCD_Cursor(object5position);
			LCD_WriteData(6);
		}
		if (object6Flag) {
			LCD_Cursor(object6position);
			LCD_WriteData(6);
		}
		if(object7Flag) {
			LCD_Cursor(object7position);
			LCD_WriteData(3);
		}
		if (object8Flag) {
			LCD_Cursor(object8position);
			LCD_WriteData(2);
		}
		break;
		
		case GameOver :
		if (gameovercounter >= 30) {
			LCD_ClearScreen();
			LCD_Cursor(1);
			for (int i = 0; i < 7; i++) {
				LCD_WriteData(scoremsg[i]);
			}
			LCD_Cursor(8);
			LCD_WriteData(Score100sPlace + '0');
			LCD_Cursor(9);
			LCD_WriteData(Score10sPlace + '0');
			LCD_Cursor(10);
			LCD_WriteData(Score1sPlace + '0');
		}
		else {
			LCD_ClearScreen();
			LCD_Cursor(2);
			LCD_WriteData(7);
			LCD_WriteData(7);
			for (int i = 0; i < 10; i++) {
				LCD_WriteData(GameOverMsg[i]);
			}
			LCD_WriteData(7);
			LCD_WriteData(7);
		}
		gameovercounter++;
		break;
		
		case DisplayScore :
		tmpscore1 = EEPROM_read(0x04);
		tmpscore2 = EEPROM_read(0x05);
		tmpscore3 = EEPROM_read(0x06);
		LCD_Cursor(13);
		LCD_WriteData(tmpscore1 + '0');
		LCD_WriteData(tmpscore2 + '0');
		LCD_WriteData(tmpscore3 + '0');
		break;
	}
}

void TickFct_Position()
{
	switch(position_state){
		case PosStart :
		if (StartFlag) position_state = CalculatingPosition;
		else position_state = PosStart;
		break;
		
		case CalculatingPosition :
		if (StartFlag) position_state = CalculatingPosition;
		else{
			position_state = PosStart;
		}
		break;
	}
	
	switch(position_state){
		case PosStart :
		playerposition = 3;
		break;
		
		case CalculatingPosition :
		if (x < 400 ){
			playerposition = 3;
			LCD_Cursor(19);
			LCD_WriteData('_');
		}
		else if(x > 730){
			playerposition = 19;
			LCD_Cursor(3);
			LCD_WriteData('_');
		}
		break;
	}
	
}

void TickFct_Object()
{
	switch(obj_state){
		case ObjStart :
		if (StartFlag){
			obj_state = CalculatingObjLocation;
		}
		else{
			obj_state = ObjStart;
		}
		break;
		
		case CalculatingObjLocation :
		if (StartFlag) obj_state = CalculatingObjLocation;
		else {
			obj_state = ObjStart;
		}
		break;
	}
	
	switch(obj_state) {
		case CalculatingObjLocation :

		if (level1Flag) {
			if (obstacle1gonecnt == 3) object1Flag = 0;
			if (obstacle2gonecnt == 3) object2Flag = 0;

			if ((cnt > 1) && (obstacle1gonecnt != 3)) {
				LCD_Cursor(cnt);
				LCD_WriteData('_');
				cnt--;
				objectposition = cnt;
				object1Flag = 1;
				} else if ((cnt <= 1) && (obstacle1gonecnt != 3)) {
				cnt = 17;
				obstacle1gonecnt++;
			}
			if (tickcnt >= 4) {
				if ((cnt3 > 17) && (obstacle2gonecnt != 3)) {
					LCD_Cursor(cnt3);
					LCD_WriteData('_');
					cnt3--;
					object2position = cnt3;
					object2Flag = 1;
					} else if ((cnt3 <= 17) && (obstacle2gonecnt != 3)){
					cnt3 = 33;
					obstacle2gonecnt++;
				}
			}
		}

		if (level2Flag){

			if ((cnt3 > 17) && (obstacle2gonecnt != 3)) {
				LCD_Cursor(cnt3);
				LCD_WriteData('_');
				cnt3--;
				object2position = cnt3;
				object2Flag = 1;
			}
			else if((cnt3 <= 17) && (obstacle2gonecnt != 3)) {
				cnt3 = 33;
				obstacle2gonecnt++;
			}
			if (tickcnt >= 4) {
				if ((cnt > 1) && (obstacle1gonecnt != 3)) {
					LCD_Cursor(cnt);
					LCD_WriteData('_');
					cnt--;
					objectposition = cnt;
					object1Flag = 1;
					} else if ((cnt <= 1) && (obstacle1gonecnt != 3)) {
					cnt = 17;
					obstacle1gonecnt++;
				}
			}

			if (tickcnt >= 25) {
				if ((cnt2 > 1) && (obstacle3gonecnt != 2)) {
					LCD_Cursor(cnt2);
					LCD_WriteData('_');
					cnt2--;
					object3position = cnt2;
					object3Flag = 1;
					} else if ((cnt2 <= 1) && (obstacle3gonecnt != 2)) {
					cnt2 = 17;
					obstacle3gonecnt++;
				}
			}

			if (tickcnt >= 28) {
				if ((cnt4 > 17) && (obstacle4gonecnt != 2)) {
					LCD_Cursor(cnt4);
					LCD_WriteData('_');
					cnt4--;
					object4position = cnt4;
					object4Flag = 1;
				}
				else if((cnt4 <= 17) && (obstacle4gonecnt != 2)) {
					cnt4 = 33;
					obstacle4gonecnt++;
				}
			}
			if (obstacle2gonecnt == 3) object2Flag = 0;
			if (obstacle1gonecnt == 3) object1Flag = 0;
			if (obstacle3gonecnt == 2) object3Flag = 0;
			if (obstacle4gonecnt == 2) object4Flag = 0;

		}

		if (level3Flag) {
			if ((cnt > 1) && (obstacle1gonecnt != 3)) {
				LCD_Cursor(cnt);
				LCD_WriteData('_');
				cnt--;
				objectposition = cnt;
				object1Flag = 1;
			}
			else if ((cnt <= 1) && (obstacle1gonecnt != 3)) {
				cnt = 17;
				obstacle1gonecnt++;
				LCD_Cursor(1);
				LCD_WriteData('_');
			}

			if (tickcnt >= 14) {
				if ((cnt8 > 1) && (obstacle3gonecnt != 3)) {
					LCD_Cursor(cnt8);
					LCD_WriteData('_');
					cnt8--;
					object3position = cnt8;
					object3Flag = 1;
				}
				else if((cnt8 <= 1) && (obstacle3gonecnt != 3)) {
					cnt8 = 17;
					obstacle3gonecnt++;
					LCD_Cursor(1);
					LCD_WriteData('_');
				}
			}

			if (tickcnt >= 6) {
				if ((cnt3 > 17) && (obstacle6gonecnt != 2)) {
					LCD_Cursor(cnt3);
					LCD_WriteData('_');
					cnt3--;
					object6position = cnt3;
					object6Flag = 1;
				}
				else if ((cnt <= 17) && (obstacle6gonecnt != 2)) {
					cnt3 = 33;
					obstacle6gonecnt++;
					LCD_Cursor(17);
					LCD_WriteData('_');
				}
			}

			if (tickcnt >= 28) {
				if ((cnt4 > 17) && (obstacle4gonecnt != 2)) {
					LCD_Cursor(cnt4);
					LCD_WriteData('_');
					cnt4--;
					object4position = cnt4;
					object4Flag = 1;
				}
				else if ((cnt4 <= 17) && (obstacle4gonecnt != 2)) {
					cnt4 = 33;
					obstacle4gonecnt++;
					LCD_Cursor(17);
					LCD_WriteData('_');
				}
			}

			if (obstacle1gonecnt == 3) object1Flag = 0;
			if (obstacle3gonecnt == 3) object3Flag = 0;
			if (obstacle6gonecnt == 2) object6Flag = 0;
			if (obstacle4gonecnt == 2) object4Flag = 0;
		}

		if (level1Flag && (obstacle1gonecnt + obstacle2gonecnt == 6)){
			level1Flag = 0;
			level2Flag = 1;
			obstacle1gonecnt = 0;
			obstacle2gonecnt = 0;
			cnt = 17;
			cnt3 = 33;
			tickcnt = 0;
		}

		if (level2Flag && (obstacle2gonecnt + obstacle1gonecnt + obstacle3gonecnt + obstacle4gonecnt == 10)) {
			level2Flag = 0;
			level3Flag = 1;
			obstacle2gonecnt = 0;
			obstacle1gonecnt = 0;
			obstacle3gonecnt = 0;
			obstacle4gonecnt = 0;
			LCD_Cursor(1);
			LCD_WriteData('_');
			LCD_Cursor(17);
			LCD_WriteData('_');
			cnt = 17;
			cnt2 = 17;
			cnt3 = 33;
			cnt4 = 33;
			tickcnt = 0;
		}

		if ((level3Flag) && ((obstacle1gonecnt + obstacle3gonecnt + obstacle6gonecnt + obstacle4gonecnt) == 10)) {
			level3Flag = 0;
			level1Flag = 1;
			obstacle1gonecnt = 0;
			obstacle3gonecnt = 0;
			obstacle6gonecnt = 0;
			obstacle4gonecnt = 0;
			cnt8 = 17;
			cnt = 17;
			cnt3 = 33;
			cnt4 = 33;
			tickcnt = 0;
		}

		tickcnt++;
		break;
	}
}

void TickFct_HitDetect()
{
	
	switch(hit_state){
		case HitStart :
		if (StartFlag) hit_state = CalculatingHitDetect;
		else hit_state = HitStart;
		break;
		
		case CalculatingHitDetect :
		if (!StartFlag) {
			hit_state = HitStart;
		}
		else if (HitFlag){
			hit_state = Stop;
		}
		else hit_state = CalculatingHitDetect;
		break;
		
		case Stop :
		if (StartFlag) hit_state = Stop;
		else hit_state = HitStart;
		
		default :
		hit_state = HitStart;
		break;
	}
	
	switch(hit_state){
		case HitStart :
		HitFlag = 0;
		break;
		
		case CalculatingHitDetect :
		if (object1Flag && (playerposition == objectposition)) HitFlag = 1;
		if (object2Flag && (playerposition == object2position)) HitFlag = 1;
		if (object3Flag && (playerposition == object3position)) HitFlag = 1;
		if (object4Flag && (playerposition == object4position)) HitFlag = 1;
		if (object5Flag && (playerposition == object5position)) HitFlag = 1;
		if (object6Flag && (playerposition == object6position)) HitFlag = 1;
		if (object7Flag && (playerposition == object7position)) HitFlag = 1;
		if (object8Flag && (playerposition == object8position)) HitFlag = 1;
		break;
	}
	
}


int main (void)
{
	DDRD = 0xFF; PORTD = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRA = 0x00; PORTA = 0xFF;
	
	
	TimerSet(150);
	TimerOn();
	
	LCD_init();
	LCD_WriteCommand(0x0C);
	CreateCustomChar(CustomCharPlayer, 1);
	CreateCustomChar(CustomCharRock, 2);
	CreateCustomChar(CustomCharEnemy, 3);
	CreateCustomChar(CustomCharLeftFlag, 4);
	CreateCustomChar(CustomCharRightFlag, 5);
	CreateCustomChar(CustomCharTree, 6);
	CreateCustomChar(CustomCharSadFace, 7);
	LCD_WriteCommand(0x80);
	
	ADC_init();
	
	testoutput_state = Start;
	position_state = PosStart;
	obj_state = ObjStart;
	hit_state = HitStart;
	score_state = ScoreStart;
	
	while(1){
		x = ADC;
		TickFct_Object();
		TickFct_TestOutput();
		TickFct_Position();
		TickFct_HitDetect();
		TickFct_Scoring();
		while(!TimerFlag){}
		TimerFlag = 0;
	}
}
