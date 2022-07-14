# Grabs the 2 LSBs from each byte from offset d54 to d561049 then exports to an image
# NOTE: This code is a mess, but it works!
import binascii

def get_file_bytes_hex(fileName):
	tmpStorage = []
	byteCount = 0
	with open(fileName, "rb") as f:
		for byte in iter(lambda: f.read(1), b''):
			byte = binascii.hexlify(byte)
			tmpStorage.append(byte)
			byteCount = byteCount + 1
	print("Total bytes for file", fileName,":",byteCount)
	return tmpStorage

def extract_files(bmp1,bmp2):
	#We know that the encoded image starts at offset d54 (the first pixel after the BMP header info) and ends at offset d561049 
	#Ideally, we shound't use magic numbers.
	bytesLSB = []
	for x in range(54,561050):
		#This will be SLOW - would be much faster either via buffer or in C
		if bmp1[x] != bmp2[x]:
			bits = (int(bmp2[x],16) - int(bmp1[x],16))
			bits = ('{0:b}'.format(bits).zfill(8))
			bytesLSB.append(bits[-2:])
		
		else:
			bytesLSB.append('00')

	bytesLSB = (''.join(x) for x in zip(bytesLSB[0::2], bytesLSB[1::2]))
	bytesLSB = list(bytesLSB)

	for x in range(len(bytesLSB)):
		with open("Output2.bin",'a') as f:
			writeStr = ("{} \n".format(bytesLSB[x]))
			f.write(writeStr)

	for byte in range(len(bytesLSB)):
		bytesLSB[byte] = '{0:X}'.format(int(bytesLSB[byte],2))

	x = (''.join(bytesLSB))

	with open("Output.jpg",'ab') as f:
		f.write(bytes.fromhex(x))

def main():
	print("### Loading File1.bmp... ###")
	bmp1 = get_file_bytes_hex("File1.bmp")
	print("Done!\n### Loading File2.bmp... ###")
	bmp2 = get_file_bytes_hex("File2.bmp")
	print("### Both loaded! Now extracting and creating binary list... ###")
	extract_files(bmp1,bmp2)
	print("### Completed! Look for Output.jpg ###")

if __name__ == "__main__":
	main()