/*
============================================================================
 Name        : 	Md5Wrapper.h
 Author      : 	Ross Savage
 Copyright   : 	2010 Buddycloud
 Description : 	Wrapper class for CMD5
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef MD5WRAPPER_H_
#define MD5WRAPPER_H_

#include <hash.h>

class CMD5Wrapper : public CBase {
	public:
		static CMD5Wrapper* NewLC();
		virtual ~CMD5Wrapper();
		
	private:
		void ConstructL();
	
	public:
		void Update(const TDesC8 &aMessage);	
		TPtrC8 Final();	
	
	public:
		TPtrC8 FinalHex(const TDesC8 &aMessage);
		TPtrC8 FinalHex();
		
	protected:
		CMD5* iMd5;
		
		HBufC8* iHex;
};

#endif /* MD5WRAPPER_H_ */
