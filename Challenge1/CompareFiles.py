#Compares the two BMP files and determines which values are different at each offset
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


def compare_files(bmp1,bmp2):
	byteDiff = []
	# Ideally, we shouldn't use magic numbers. 2880294 comes from get_file_bytes_hex
	for x in range(2880294):
		#This will be SLOW - would be much faster either via buffer or in C
		if bmp1[x] != bmp2[x]:
			print("### DIFFERENT BYTE AT DECIMAL OFFSET:",x)
			print("File1.bmp Value: ",bmp1[x])
			print("File2.bmp Value: ",bmp2[x])
			byte1 = ('{0:b}'.format(int(bmp1[x],16)).zfill(8))
			byte2 = ('{0:b}'.format(int(bmp2[x],16)).zfill(8))
			print(byte1,"\n"+byte2)
			
	with open("Output.bin",'a') as f:
		for x in range(2880294):
			writeStr = "Offset: {} File1 Value: {} File2 Value: {}\n".format(x,bmp1[x],bmp2[x])
			f.write(writeStr)
def main():
	print("### Loading File1.bmp... ###")
	bmp1 = get_file_bytes_hex("File1.bmp")
	print("Done!\n### Loading File2.bmp... ###")
	bmp2 = get_file_bytes_hex("File2.bmp")
	print("### Both loaded! Now comparing... ###")
	compare_files(bmp1,bmp2)
	print("### Completed! Look for Output.bin ###")

if __name__ == "__main__":
	main()