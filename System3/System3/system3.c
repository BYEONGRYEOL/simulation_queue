#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#include <stdlib.h>
#include <stdio.h>  
#include <math.h>
#include "lcgrand.h"

#define Q_LIMIT 400
#define KIOSK1_Q_LIMIT 200
#define KIOSK2_Q_LIMIT 200
#define BUSY      1
#define IDLE      0


//main�� ������ ���� ���� 
int seed, num_events;
float time_end;
float interarrival_k, interarrival_sigma, interarrival_mu, interarrival_payco_prob;
float kiosk_gamma, kiosk_delta, kiosk_lambda, kiosk_zeta;
float server_k, server_alpha, server_beta;

//initialize�� ������ ���� ���� 
float sim_time, time_next_event[5];
int next_event_type;

//arrive�� ������ ���� ����
int cust_payco ;


//kiosk_1_arrive�� ������ ���� ����
int num_in_kiosk1_q, num_custs_kiosk1_delayed, kiosk1_status, max_num_in_kiosk1_q;
float time_kiosk1_arrival[KIOSK1_Q_LIMIT + 1], total_of_kiosk1_delays;
int kiosk1_ispayco[KIOSK1_Q_LIMIT + 1];

int num_in_kiosk2_q, num_custs_kiosk2_delayed, kiosk2_status, max_num_in_kiosk2_q;
float time_kiosk2_arrival[KIOSK2_Q_LIMIT + 1], total_of_kiosk2_delays;
int kiosk2_ispayco[KIOSK2_Q_LIMIT + 1];

//update_time_avg_stats�� ������ ���� ����
float area_num_in_kiosk1_q, area_num_in_kiosk2_q, area_kiosk1_status, area_kiosk2_status, time_last_event;

//server depart�� ������ ���� ����
int kiosk1_num_delay_over_3, kiosk1_num_delay_over_4;
float max_kiosk1_delay;

//kiosk depart�� ������ ���� ����
int kiosk2_num_delay_over_3, kiosk2_num_delay_over_4;
float max_kiosk2_delay;



//file ���� ���� 
FILE* infile, * outfile;

//declare function
//declare random generate function
int is_cust_payco(void);
float cust_arrival_pareto(float interarrival_k, float interarrival_sigma, float interarrival_mu);

float normal_service(float server_k, float server_alpha, float server_beta);

float kiosk_service_AR(float kiosk_gamma, float kiosk_delta, float kiosk_lambda, float kiosk_zeta);


//declare function
void initialize(void);
void timing();
void update_time_avg_stats(void);
void arrive(void);
void report(void);

void kiosk2_arrive(int);
void kiosk1_arrive(int);
void kiosk1_depart(void);
void kiosk2_depart(void);

int main(void) {

	//���� �а� ���� 
	infile = fopen("system3_input.in", "r");
	outfile = fopen("system3_output13.out", "w");

	//event 1 = �� ���� 2 = Ű����ũ1�� �� ���, 3 = Ű����ũ2�� �� ���, 4 = fake event 
	num_events = 4;

	//���Ͽ��� �� �о���� 
	fscanf(infile, "%f %f %f %f %f %f %f %f %f %f %f %f %d", 
		&interarrival_payco_prob, &interarrival_k, &interarrival_sigma, &interarrival_mu,  
		&kiosk_gamma, &kiosk_delta, &kiosk_lambda, &kiosk_zeta, 
		&server_k, &server_alpha, &server_beta, &time_end, &seed);

	//outfile�� ���� parameter �� �ľ�
	fprintf(outfile, "Ű����ũ 2��\n\n");
	fprintf(outfile, "******************************************************************************************\n");
	fprintf(outfile, "�� ���� pareto k : \t\t\t\t\t\t %11.3f \n\n", interarrival_k);
	fprintf(outfile, "�� ���� pareto sigma : \t\t\t\t\t\t %11.3f \n\n", interarrival_sigma);
	fprintf(outfile, "�� ���� pareto mu : \t\t\t\t\t\t %11.3f \n\n", interarrival_mu);
	fprintf(outfile, "������ ���� payco ������ ���� Ȯ�� \t\t\t\t %16.4f \n\n", interarrival_payco_prob);
	fprintf(outfile, "kiosk_Johnson SB gamma: \t\t\t\t\t\t\t\t\t\t %11.3f \n\n", kiosk_gamma);
	fprintf(outfile, "kiosk_Johnson SB delta: \t\t\t\t\t\t\t\t\t\t %11.3f \n\n", kiosk_delta);
	fprintf(outfile, "kiosk_Johnson SB lambda: \t\t\t\t\t\t\t\t\t\t %11.3f \n\n", kiosk_lambda);
	fprintf(outfile, "kiosk_Johnson SB zeta: \t\t\t\t\t\t\t\t\t\t %11.3f \n\n", kiosk_zeta);
	fprintf(outfile, "server_Dagum k : \t\t\t\t\t\t %11.3f \n\n", server_k);
	fprintf(outfile, "server_Dagum alpha: \t\t\t\t\t\t %11.3f \n\n", server_alpha);
	fprintf(outfile, "server_Dagum beta : \t\t\t\t\t\t %11.3f \n\n", server_beta);
	fprintf(outfile, "�ùķ��̼� �� �ð� \t\t\t\t\t\t %14f\n\n", time_end);
	fprintf(outfile, "Random Generate Seed Value \t\t\t\t\t %14d\n\n", seed);


	//initialize 
	initialize();

	do {
		timing();

		update_time_avg_stats();

		switch (next_event_type) {
		case 1:
			arrive();
			break;

		case 2:
			kiosk1_depart();
			break;

		case 3:
			kiosk2_depart();
			break;

		case 4:
			report();
			break;
		}
	} while (next_event_type != 4);


	fclose(infile);
	fclose(outfile);

	return 0;
}

void initialize(void) {

	sim_time = 0.0;

	kiosk1_status = IDLE;
	kiosk2_status = IDLE;

	max_num_in_kiosk1_q = 0;
	max_num_in_kiosk2_q = 0;
	max_kiosk1_delay = 0.0;
	max_kiosk2_delay = 0.0;

	num_in_kiosk1_q = 0;
	num_custs_kiosk1_delayed = 0;
	total_of_kiosk1_delays = 0.0;

	num_in_kiosk2_q = 0;
	num_custs_kiosk2_delayed = 0;
	total_of_kiosk2_delays = 0;

	time_last_event = 0.0;
	area_num_in_kiosk1_q = 0.0;
	area_num_in_kiosk2_q = 0.0;
	area_kiosk1_status = 0.0;
	area_kiosk2_status = 0.0;

	kiosk1_num_delay_over_3 = 0;
	kiosk1_num_delay_over_4 = 0;

	kiosk2_num_delay_over_3 = 0;
	kiosk2_num_delay_over_4 = 0;

	

	//event list �ʱ�ȭ	
	time_next_event[1] = sim_time + cust_arrival_pareto(interarrival_k, interarrival_sigma, interarrival_mu);
	time_next_event[2] = 1.0e+30;
	time_next_event[3] = 1.0e+30;
	time_next_event[4] = time_end;
}

void timing() {
	int   i;
	float min_time_next_event = 1.0e+29;

	next_event_type = 0;

	for (i = 1; i <= num_events; ++i) {
		if (time_next_event[i] < min_time_next_event) {
			min_time_next_event = time_next_event[i];
			next_event_type = i;
		}
	}
	//event list ������� Ȯ�� 
	if (next_event_type == 0) {
		//error message ��� 
		fprintf(outfile, "\nEvent list empty at time %f", sim_time);
		exit(1);
	}

	//event list �Ⱥ������ sim_time ���� 
	sim_time = min_time_next_event;

}

void update_time_avg_stats(void) {
	float time_since_last_event;

	//time_since_last_event ��� & time_last_event ������Ʈ 
	time_since_last_event = sim_time - time_last_event;
	time_last_event = sim_time;

	area_num_in_kiosk1_q += num_in_kiosk1_q * time_since_last_event;
	area_num_in_kiosk2_q += num_in_kiosk2_q * time_since_last_event;

	/* Update area under server-busy indicator function. */
	area_kiosk1_status += kiosk1_status * time_since_last_event;
	area_kiosk2_status += kiosk2_status * time_since_last_event;
	
}

void arrive(void) {
	//���� arrival event ���� 
	
	//time_next_event[1] = sim_time + cust_arrival(interarrival_alpha, interarrival_beta);
	time_next_event[1] = sim_time + cust_arrival_pareto(interarrival_k, interarrival_sigma, interarrival_mu);

	
	//payco ������?
	cust_payco = is_cust_payco();

	// ���� �� ª�� ���� Ű����ũ�� ��������� ����
	if (num_in_kiosk1_q > num_in_kiosk2_q) {
		kiosk2_arrive(cust_payco);
	}
	else if (num_in_kiosk1_q == num_in_kiosk2_q) {
		if (lcgrand(seed) >= 0.5) {
			kiosk1_arrive(cust_payco);
		}
		else
			kiosk2_arrive(cust_payco);
	}
	else
		kiosk1_arrive(cust_payco);
	
	
	
}


// payco �������̸� 1, �Ϲݰ������̸� 0
void kiosk1_arrive(int payco) {

	float kiosk1_delay, kiosk1_servicetime;

	if (payco == 1)
		kiosk1_servicetime = kiosk_service_AR(kiosk_gamma, kiosk_delta, kiosk_lambda, kiosk_zeta);
	else
		kiosk1_servicetime = normal_service(server_k, server_alpha, server_beta);
	

	if (kiosk1_status == BUSY) {
		//Ű����ũ busy -> Ű����ũ queue�� �� 1�� ���� 
		++num_in_kiosk1_q;
		
		if (max_num_in_kiosk1_q <= num_in_kiosk1_q) {
			max_num_in_kiosk1_q = num_in_kiosk1_q;
		}
		
		// Ű����ũ queue overflow?
		if (num_in_kiosk1_q > KIOSK1_Q_LIMIT) {
			//�ߴ� 
			fprintf(outfile, "\nŰ����ũ1 QUEUE OVERFLOW �ð��� ");
			fprintf(outfile, " %f", sim_time);
			exit(2);
		}

		//Ű����ũ ���� ���� ���� �ð�
		time_kiosk1_arrival[num_in_kiosk1_q] = sim_time;
		
		// ����ϴ� ���� ���������������� ����
		kiosk1_ispayco[num_in_kiosk1_q] = payco;

	}
	else {
		//Ű����ũ IDLE -> ������ ���� �ٷ� ���� ���� 
		kiosk1_delay = 0.0;
		total_of_kiosk1_delays += kiosk1_delay;
		//Ű����ũ ���� delay ��ģ �� 1�� ���� & Ű����ũ ���� BUSY�� 
		++num_custs_kiosk1_delayed;
		kiosk1_status = BUSY;

		//kiosk_depart �ð� ���� 
		time_next_event[2] = sim_time + kiosk1_servicetime;
	}
}


void kiosk1_depart() {
	int i;
	float kiosk1_delay;
	float kiosk1_servicetime;

	if (num_in_kiosk1_q == 0) {
		//queue�� ��������ϱ� Ű����ũ IDLE & Ű����ũ ��� �ð� ���� ���� 
		kiosk1_status = IDLE;
		time_next_event[2] = 1.0e+30;
	}
	else {
		--num_in_kiosk1_q;
		//���� Ű����ũ delay �ð� & ���� Ű����ũ  delay �ð� ���
		kiosk1_delay = sim_time - time_kiosk1_arrival[1];

		
		total_of_kiosk1_delays += kiosk1_delay;

		if (max_kiosk1_delay <= kiosk1_delay) {
			max_kiosk1_delay = kiosk1_delay;
		}
		

		//Ű����ũ delay �ð��� 3��(180��) �̻��� ��� �� ����
		
		if (kiosk1_delay >= 180) {
			++kiosk1_num_delay_over_3;
		}

		//Ű����ũ delay �ð��� 4��(240��) �̻��� ��� �� ����
		if (kiosk1_delay >= 240) {
			++kiosk1_num_delay_over_4;
		}
		
		++num_custs_kiosk1_delayed;
		
		if (kiosk1_ispayco[1] == 1)
			kiosk1_servicetime = kiosk_service_AR(kiosk_gamma, kiosk_delta, kiosk_lambda, kiosk_zeta);
		else
			kiosk1_servicetime = normal_service(server_k, server_alpha, server_beta);
		time_next_event[2] = sim_time + kiosk1_servicetime;

		//�� ���� �ð� �� ĭ�� �մ��� 
		for (i = 1; i <= num_in_kiosk1_q; ++i)
			time_kiosk1_arrival[i] = time_kiosk1_arrival[i + 1];
	}
}
// payco �������̸� 1, �Ϲݰ������̸� 0
void kiosk2_arrive(int payco) {

	float kiosk2_delay, kiosk2_servicetime;

	if (payco == 1)
		kiosk2_servicetime = kiosk_service_AR(kiosk_gamma, kiosk_delta, kiosk_lambda, kiosk_zeta);
	else
		kiosk2_servicetime = normal_service(server_k, server_alpha, server_beta);


	if (kiosk2_status == BUSY) {
		//Ű����ũ busy -> Ű����ũ queue�� �� 1�� ���� 
		++num_in_kiosk2_q;

		if (max_num_in_kiosk2_q <= num_in_kiosk2_q) {
			max_num_in_kiosk2_q = num_in_kiosk2_q;
		}

		// Ű����ũ queue overflow?
		if (num_in_kiosk2_q > KIOSK2_Q_LIMIT) {
			//�ߴ� 
			fprintf(outfile, "\nŰ����ũ QUEUE OVERFLOW �ð��� ");
			fprintf(outfile, " %f", sim_time);
			exit(2);
		}

		//Ű����ũ ���� ���� ���� �ð�
		time_kiosk2_arrival[num_in_kiosk2_q] = sim_time;

		// ����ϴ� ���� ���������������� ����
		kiosk2_ispayco[num_in_kiosk2_q] = payco;

	}
	else {
		//Ű����ũ IDLE -> ������ ���� �ٷ� ���� ���� 
		kiosk2_delay = 0.0;
		total_of_kiosk2_delays += kiosk2_delay;
		//Ű����ũ ���� delay ��ģ �� 1�� ���� & Ű����ũ ���� BUSY�� 
		++num_custs_kiosk2_delayed;
		kiosk2_status = BUSY;

		//kiosk2_depart �ð� ���� 
		time_next_event[3] = sim_time + kiosk2_servicetime;
	}
}


void kiosk2_depart() {
	int i;
	float kiosk2_delay;
	float kiosk2_servicetime;

	if (num_in_kiosk2_q == 0) {
		//queue�� ��������ϱ� Ű����ũ IDLE & Ű����ũ ��� �ð� ���� ���� 
		kiosk2_status = IDLE;
		time_next_event[3] = 1.0e+30;
	}
	else {
		--num_in_kiosk2_q;
		//���� Ű����ũ delay �ð� & ���� Ű����ũ  delay �ð� ���
		kiosk2_delay = sim_time - time_kiosk2_arrival[1];


		total_of_kiosk2_delays += kiosk2_delay;

		if (max_kiosk2_delay <= kiosk2_delay) {
			max_kiosk2_delay = kiosk2_delay;
		}


		//Ű����ũ delay �ð��� 3��(180��) �̻��� ��� �� ����

		if (kiosk2_delay >= 180) {
			++kiosk2_num_delay_over_3;
		}

		//Ű����ũ delay �ð��� 4��(240��) �̻��� ��� �� ����
		if (kiosk2_delay >= 240) {
			++kiosk2_num_delay_over_4;
		}

		++num_custs_kiosk2_delayed;

		if (kiosk2_ispayco[1] == 1)
			kiosk2_servicetime = kiosk_service_AR(kiosk_gamma, kiosk_delta, kiosk_lambda, kiosk_zeta);
		else
			kiosk2_servicetime = normal_service(server_k, server_alpha, server_beta);
		time_next_event[3] = sim_time + kiosk2_servicetime;

		//�� ���� �ð� �� ĭ�� �մ��� 
		for (i = 1; i <= num_in_kiosk2_q; ++i)
			time_kiosk2_arrival[i] = time_kiosk2_arrival[i + 1];
	}
}
void report() {
	fprintf(outfile, "\n\n\n*****************************************************************************************************************\n");
	fprintf(outfile, "OUTPUT Ȯ��\n");
	fprintf(outfile, "\n\nŰ����ũ1 QUEUE�� ��� delay �ð� \t\t\t\t\t %11.3f seconds\n\n", total_of_kiosk1_delays / num_custs_kiosk1_delayed);
	fprintf(outfile, "Ű����ũ2 QUEUE�� ��� delay �ð� \t\t\t\t\t %11.3f seconds\n\n", total_of_kiosk2_delays / num_custs_kiosk2_delayed);
	fprintf(outfile, "��ü QUEUE�� ��� delay �ð� \t\t\t\t\t %11.3f seconds\n\n", (total_of_kiosk1_delays + total_of_kiosk2_delays) / (num_custs_kiosk1_delayed + num_custs_kiosk2_delayed));
	fprintf(outfile, "Ű����ũ1 QUEUE�� �ִ� delay �ð� \t\t\t\t\t %11.3f seconds\n\n", max_kiosk1_delay);
	fprintf(outfile, "Ű����ũ2 QUEUE�� �ִ� delay �ð� \t\t\t\t %11.3f seconds\n\n", max_kiosk2_delay);
	fprintf(outfile, "Ű����ũ1 QUEUE�� ��� ��� �� \t\t\t\t\t\t %10.3f��\n\n", area_num_in_kiosk1_q / (sim_time));
	fprintf(outfile, "Ű����ũ2 QUEUE�� ��� ��� �� \t\t\t\t\t %10.3f��\n\n", area_num_in_kiosk2_q / (sim_time));
	fprintf(outfile, "��ü QUEUE�� ��� ��� �� \t\t\t\t\t\t %10.3f��\n\n", (area_num_in_kiosk1_q + area_num_in_kiosk2_q) / (2 * sim_time));
	fprintf(outfile, "Ű����ũ1 QUEUE�� �ִ� ��� �� \t\t\t\t\t\t %10.3d�� \n\n", max_num_in_kiosk1_q);
	fprintf(outfile, "Ű����ũ2 QUEUE�� �ִ� ��� �� \t\t\t\t\t %10.3d�� \n\n", max_num_in_kiosk2_q);
	fprintf(outfile, "Ű����ũ1 Utilization \t\t\t\t\t\t %15.3f\n\n", area_kiosk1_status / (sim_time ));
	fprintf(outfile, "Ű����ũ2 Utilization \t\t\t\t\t %15.3f\n\n", area_kiosk2_status / (sim_time ));
	fprintf(outfile, "Ű����ũ1 QUEUE���� delay�� ��ģ �� �� \t\t\t\t %d ��\n\n", num_custs_kiosk1_delayed);
	fprintf(outfile, "Ű����ũ2 QUEUE���� delay�� ��ģ �� �� \t\t\t\t %d ��\n\n", num_custs_kiosk2_delayed);
	fprintf(outfile, "��ü system���� delay�� ��ģ �� �� \t\t\t\t %d ��\n\n", num_custs_kiosk1_delayed + num_custs_kiosk2_delayed);
	fprintf(outfile, "Ű����ũ1���� delay �ð��� 3��(180��) �̻��� ��� ���� ���� \t\t\t %f %%\n\n", (float)(kiosk1_num_delay_over_3) / (float)(num_custs_kiosk1_delayed));
	fprintf(outfile, "Ű����ũ1���� delay �ð��� 4��(240��) �̻��� ��� ���� ���� \t\t\t %f %%\n\n", (float)(kiosk1_num_delay_over_4) / (float)(num_custs_kiosk1_delayed));
	fprintf(outfile, "Ű����ũ2���� delay �ð��� 3��(180��) �̻��� ��� ���� ���� \t\t %f %%\n\n", (float)(kiosk2_num_delay_over_3) / (float)(num_custs_kiosk2_delayed));
	fprintf(outfile, "Ű����ũ2���� delay �ð��� 4��(240��) �̻��� ��� ���� ���� \t\t %f %%\n\n", (float)(kiosk2_num_delay_over_4) / (float)(num_custs_kiosk2_delayed));
	fprintf(outfile, "��ü �ý��ۿ��� 3�� �̻� ��ٸ� ����� ���� \t\t\t\t %f %% \n\n", (float)(kiosk1_num_delay_over_3 + kiosk2_num_delay_over_3) / (float)(num_custs_kiosk1_delayed + num_custs_kiosk2_delayed));
	fprintf(outfile, "��ü �ý��ۿ��� 4�� �̻� ��ٸ� ����� ���� \t\t\t\t %f %% \n\n", (float)(kiosk1_num_delay_over_4 + kiosk2_num_delay_over_4) / (float)(num_custs_kiosk1_delayed + num_custs_kiosk2_delayed));

}


//Random number generation

// payco ���������� �ް��� �ϴ� ������ ����
int is_cust_payco() {
	int i;
	float u;

	u = lcgrand(seed);

	if (u >= interarrival_payco_prob)
		i = 1;
	else
		i = 0;
	return i;
}

// ���� ���� ���� ����, Generalized Pareto ������ Inverse Transform ����� �̿��Ͽ���.
float cust_arrival_pareto(float interarrival_k, float interarrival_sigma, float interarrival_mu) {

	float u, x;
	u = lcgrand(seed);
	x = (interarrival_sigma / interarrival_k) * (pow(u, -interarrival_k) - 1.0) + interarrival_mu;

	return x;
}

// payco ������ ���� �ʴ� �Ϲ� ���� �� ���� �ð� ���� ���� Dagum ������ Inverse Transform ����� �̿��Ͽ���.
float normal_service(float server_k, float server_alpha, float server_beta) {
	float u, x;
	u = lcgrand(seed);
	x = server_beta * pow((pow((1.0 / u), 1 / server_k) - 1.0) , (1.0 / server_alpha));
	return x;
}

// payco ������ �޴� ���� �� ���� �ð� ���� ����, Johnson SB ������ ������, Acceptance-Rejection ����� �̿��Ͽ���.
float kiosk_service_AR(float kiosk_gamma, float kiosk_delta, float kiosk_lambda, float kiosk_zeta) {
	float u, x;
	u = lcgrand(seed);
	x = lcgrand(seed) * kiosk_lambda + kiosk_zeta;
	float fx = (kiosk_delta / (kiosk_lambda * pow(2 * M_PI, 1.0 / 2) * ((x - kiosk_zeta) / kiosk_lambda) * ((1.0 - (x - (double)kiosk_zeta)) / kiosk_lambda - x + kiosk_zeta))) * pow(M_E, (-1.0 / 2) * pow(kiosk_gamma + kiosk_delta * log((x - kiosk_zeta) / (kiosk_lambda - x + kiosk_zeta)), 2.0));
	while (u <= fx / 19.25714) {
		x = lcgrand(seed)* kiosk_lambda + kiosk_zeta;
	}
	return x;
			
}



