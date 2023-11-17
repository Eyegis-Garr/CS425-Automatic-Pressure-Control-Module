#include <bits/stdc++.h>


//function to find the one's complement
std::string Ones_complement(std::string data)
{
	for (int i = 0; i < data.length(); i++) {
		if (data[i] == '0')
			data[i] = '1';
		else
			data[i] = '0';
	}

	return data;
}

//function to return the checksum value
std::string checkSum(std::string data, int block_size)
{
	//check if data size is divisible by block_size
	//otherwise add '0' in front of the data
	int data_length = data.length();
	if (data_length % block_size != 0) {
		int pad_size = block_size - (data_length % block_size);
		for (int i = 0; i < pad_size; i++) {
			data = '0' + data;
		}
	}

	//binary addition of all blocks with carry
	std::string result = "";

	//girst block of data stored in result variable
	for (int i = 0; i < block_size; i++) {
		result += data[i];
	}

	//loop to calculate the block wise addition of data
	for (int i = block_size; i < data_length; i += block_size) {

		//stores the data of the next block
		std::string next_block = "";

		for (int j = i; j < i + block_size; j++) {
			next_block += data[j];
		}


		//stores the binary addition of two blocks
		std::string additions = "";
		int sum = 0;
        int carry = 0;

		//loop to calculate the binary addition of
		//the current two blocks
		for (int k = block_size - 1; k >= 0; k--) {
			sum += (next_block[k] - '0') + (result[k] - '0');
			carry = sum / 2;
			if (sum == 0) {
				additions = '0' + additions;
				sum = carry;
			}
			else if (sum == 1) {
				additions = '1' + additions;
				sum = carry;
			}
			else if (sum == 2) {
				additions = '0' + additions;
				sum = carry;
			}
			else {
				additions = '1' + additions;
				sum = carry;
			}
		}


		//after binary add of two blocks with carry,
		//if carry is 1 then apply binary addition
		std::string final = "";

		if (carry == 1) {
			for (int h = additions.length() - 1; h >= 0; h--) {
				if (carry == 0) {
					final = additions[h] + final;
				}
				else if (((additions[h] - '0') + carry) % 2 == 0) {
					final = "0" + final;
					carry = 1;
				}
				else {
					final = "1" + final;
					carry = 0;
				}
			}

			result = final;
		}
		else {
			result = additions;
		}
	}

	//return one's complements of result value
	return Ones_complement(result);
}


//function to check if the received message
//is same as the senders message (works for int and doubles)
bool checker(double sent_message, double recv_message, int block_size)
{
    std::string sent_message_string = std::bitset<64>(sent_message).to_string();
	std::string recv_message_string = std::bitset<64>(recv_message).to_string();


	//checksum Value of the senders message
	std::string sender_checksum = checkSum(sent_message_string, block_size);


	//checksum value for the receivers message
	std::string receiver_checksum = checkSum(recv_message_string + sender_checksum, block_size);


	//if receivers checksum value is 0
	if (count(receiver_checksum.begin(), receiver_checksum.end(), '0') == block_size) {
		return true;
	}
	else {
		return false;
	}
}

int main()
{

    //works for int and double
	double sent_message[] = {1.1, 3, 4.56};
	double recv_message[] = {1.1, 2, 4.58};

    int array_length = sizeof(sent_message)/sizeof(*sent_message);
    std::cout << array_length << std::endl;

	int block_size = 8;

    for (int i = 0; i < array_length; i++) {
        if (checker(sent_message[i], recv_message[i], block_size)) {
            std::cout << "No Error" << std::endl;
        }
        else {
            std::cout << "Error" << std::endl;
        }
    }

	return 0;
}

/*
Things to think about or ask

need to find if main code wants to pass in two values and check if they are the same, 
or pass in one value and return checksum then have another function to check checksums

*/