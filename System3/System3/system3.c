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


//main에 나오는 변수 선언 
int seed, num_events;
float time_end;
float interarrival_k, interarrival_sigma, interarrival_mu, interarrival_payco_prob;
float kiosk_gamma, kiosk_delta, kiosk_lambda, kiosk_zeta;
float server_k, server_alpha, server_beta;

//initialize에 나오는 변수 선언 
float sim_time, time_next_event[5];
int next_event_type;

//arrive에 나오는 변수 선언
int cust_payco ;


//kiosk_1_arrive에 나오는 변수 선언
int num_in_kiosk1_q, num_custs_kiosk1_delayed, kiosk1_status, max_num_in_kiosk1_q;
float time_kiosk1_arrival[KIOSK1_Q_LIMIT + 1], total_of_kiosk1_delays;
int kiosk1_ispayco[KIOSK1_Q_LIMIT + 1];

int num_in_kiosk2_q, num_custs_kiosk2_delayed, kiosk2_status, max_num_in_kiosk2_q;
float time_kiosk2_arrival[KIOSK2_Q_LIMIT + 1], total_of_kiosk2_delays;
int kiosk2_ispayco[KIOSK2_Q_LIMIT + 1];

//update_time_avg_stats에 나오는 변수 선언
float area_num_in_kiosk1_q, area_num_in_kiosk2_q, area_kiosk1_status, area_kiosk2_status, time_last_event;

//server depart에 나오는 변수 선언
int kiosk1_num_delay_over_3, kiosk1_num_delay_over_4;
float max_kiosk1_delay;

//kiosk depart에 나오는 변수 선언
int kiosk2_num_delay_over_3, kiosk2_num_delay_over_4;
float max_kiosk2_delay;



//file 변수 선언 
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

	//파일 읽고 쓰기 
	infile = fopen("system3_input.in", "r");
	outfile = fopen("system3_output13.out", "w");

	//event 1 = 고객 도착 2 = 키오스크1번 고객 출발, 3 = 키오스크2번 고객 출발, 4 = fake event 
	num_events = 4;

	//파일에서 값 읽어오기 
	fscanf(infile, "%f %f %f %f %f %f %f %f %f %f %f %f %d", 
		&interarrival_payco_prob, &interarrival_k, &interarrival_sigma, &interarrival_mu,  
		&kiosk_gamma, &kiosk_delta, &kiosk_lambda, &kiosk_zeta, 
		&server_k, &server_alpha, &server_beta, &time_end, &seed);

	//outfile에 분포 parameter 등 파악
	fprintf(outfile, "키오스크 2대\n\n");
	fprintf(outfile, "******************************************************************************************\n");
	fprintf(outfile, "고객 도착 pareto k : \t\t\t\t\t\t %11.3f \n\n", interarrival_k);
	fprintf(outfile, "고객 도착 pareto sigma : \t\t\t\t\t\t %11.3f \n\n", interarrival_sigma);
	fprintf(outfile, "고객 도착 pareto mu : \t\t\t\t\t\t %11.3f \n\n", interarrival_mu);
	fprintf(outfile, "도착한 고객이 payco 결제를 원할 확률 \t\t\t\t %16.4f \n\n", interarrival_payco_prob);
	fprintf(outfile, "kiosk_Johnson SB gamma: \t\t\t\t\t\t\t\t\t\t %11.3f \n\n", kiosk_gamma);
	fprintf(outfile, "kiosk_Johnson SB delta: \t\t\t\t\t\t\t\t\t\t %11.3f \n\n", kiosk_delta);
	fprintf(outfile, "kiosk_Johnson SB lambda: \t\t\t\t\t\t\t\t\t\t %11.3f \n\n", kiosk_lambda);
	fprintf(outfile, "kiosk_Johnson SB zeta: \t\t\t\t\t\t\t\t\t\t %11.3f \n\n", kiosk_zeta);
	fprintf(outfile, "server_Dagum k : \t\t\t\t\t\t %11.3f \n\n", server_k);
	fprintf(outfile, "server_Dagum alpha: \t\t\t\t\t\t %11.3f \n\n", server_alpha);
	fprintf(outfile, "server_Dagum beta : \t\t\t\t\t\t %11.3f \n\n", server_beta);
	fprintf(outfile, "시뮬레이션 총 시간 \t\t\t\t\t\t %14f\n\n", time_end);
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

	

	//event list 초기화	
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
	//event list 비었는지 확인 
	if (next_event_type == 0) {
		//error message 출력 
		fprintf(outfile, "\nEvent list empty at time %f", sim_time);
		exit(1);
	}

	//event list 안비었으면 sim_time 설정 
	sim_time = min_time_next_event;

}

void update_time_avg_stats(void) {
	float time_since_last_event;

	//time_since_last_event 계산 & time_last_event 업데이트 
	time_since_last_event = sim_time - time_last_event;
	time_last_event = sim_time;

	area_num_in_kiosk1_q += num_in_kiosk1_q * time_since_last_event;
	area_num_in_kiosk2_q += num_in_kiosk2_q * time_since_last_event;

	/* Update area under server-busy indicator function. */
	area_kiosk1_status += kiosk1_status * time_since_last_event;
	area_kiosk2_status += kiosk2_status * time_since_last_event;
	
}

void arrive(void) {
	//다음 arrival event 설정 
	
	//time_next_event[1] = sim_time + cust_arrival(interarrival_alpha, interarrival_beta);
	time_next_event[1] = sim_time + cust_arrival_pareto(interarrival_k, interarrival_sigma, interarrival_mu);

	
	//payco 적립고객?
	cust_payco = is_cust_payco();

	// 줄이 더 짧은 곳의 키오스크로 결제방식을 전달
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


// payco 결제고객이면 1, 일반결제고객이면 0
void kiosk1_arrive(int payco) {

	float kiosk1_delay, kiosk1_servicetime;

	if (payco == 1)
		kiosk1_servicetime = kiosk_service_AR(kiosk_gamma, kiosk_delta, kiosk_lambda, kiosk_zeta);
	else
		kiosk1_servicetime = normal_service(server_k, server_alpha, server_beta);
	

	if (kiosk1_status == BUSY) {
		//키오스크 busy -> 키오스크 queue에 고객 1명 증가 
		++num_in_kiosk1_q;
		
		if (max_num_in_kiosk1_q <= num_in_kiosk1_q) {
			max_num_in_kiosk1_q = num_in_kiosk1_q;
		}
		
		// 키오스크 queue overflow?
		if (num_in_kiosk1_q > KIOSK1_Q_LIMIT) {
			//중단 
			fprintf(outfile, "\n키오스크1 QUEUE OVERFLOW 시각은 ");
			fprintf(outfile, " %f", sim_time);
			exit(2);
		}

		//키오스크 결제 고객의 도착 시간
		time_kiosk1_arrival[num_in_kiosk1_q] = sim_time;
		
		// 대기하는 고객이 페이코적립고객인지 저장
		kiosk1_ispayco[num_in_kiosk1_q] = payco;

	}
	else {
		//키오스크 IDLE -> 딜레이 없이 바로 서비스 받음 
		kiosk1_delay = 0.0;
		total_of_kiosk1_delays += kiosk1_delay;
		//키오스크 에서 delay 마친 고객 1명 증가 & 키오스크 상태 BUSY로 
		++num_custs_kiosk1_delayed;
		kiosk1_status = BUSY;

		//kiosk_depart 시각 설정 
		time_next_event[2] = sim_time + kiosk1_servicetime;
	}
}


void kiosk1_depart() {
	int i;
	float kiosk1_delay;
	float kiosk1_servicetime;

	if (num_in_kiosk1_q == 0) {
		//queue가 비어있으니까 키오스크 IDLE & 키오스크 출발 시각 무한 설정 
		kiosk1_status = IDLE;
		time_next_event[2] = 1.0e+30;
	}
	else {
		--num_in_kiosk1_q;
		//고객의 키오스크 delay 시간 & 누적 키오스크  delay 시간 계산
		kiosk1_delay = sim_time - time_kiosk1_arrival[1];

		
		total_of_kiosk1_delays += kiosk1_delay;

		if (max_kiosk1_delay <= kiosk1_delay) {
			max_kiosk1_delay = kiosk1_delay;
		}
		

		//키오스크 delay 시간이 3분(180초) 이상인 사람 수 조사
		
		if (kiosk1_delay >= 180) {
			++kiosk1_num_delay_over_3;
		}

		//키오스크 delay 시간이 4분(240초) 이상인 사람 수 조사
		if (kiosk1_delay >= 240) {
			++kiosk1_num_delay_over_4;
		}
		
		++num_custs_kiosk1_delayed;
		
		if (kiosk1_ispayco[1] == 1)
			kiosk1_servicetime = kiosk_service_AR(kiosk_gamma, kiosk_delta, kiosk_lambda, kiosk_zeta);
		else
			kiosk1_servicetime = normal_service(server_k, server_alpha, server_beta);
		time_next_event[2] = sim_time + kiosk1_servicetime;

		//고객 도착 시각 한 칸씩 앞당기기 
		for (i = 1; i <= num_in_kiosk1_q; ++i)
			time_kiosk1_arrival[i] = time_kiosk1_arrival[i + 1];
	}
}
// payco 결제고객이면 1, 일반결제고객이면 0
void kiosk2_arrive(int payco) {

	float kiosk2_delay, kiosk2_servicetime;

	if (payco == 1)
		kiosk2_servicetime = kiosk_service_AR(kiosk_gamma, kiosk_delta, kiosk_lambda, kiosk_zeta);
	else
		kiosk2_servicetime = normal_service(server_k, server_alpha, server_beta);


	if (kiosk2_status == BUSY) {
		//키오스크 busy -> 키오스크 queue에 고객 1명 증가 
		++num_in_kiosk2_q;

		if (max_num_in_kiosk2_q <= num_in_kiosk2_q) {
			max_num_in_kiosk2_q = num_in_kiosk2_q;
		}

		// 키오스크 queue overflow?
		if (num_in_kiosk2_q > KIOSK2_Q_LIMIT) {
			//중단 
			fprintf(outfile, "\n키오스크 QUEUE OVERFLOW 시각은 ");
			fprintf(outfile, " %f", sim_time);
			exit(2);
		}

		//키오스크 결제 고객의 도착 시간
		time_kiosk2_arrival[num_in_kiosk2_q] = sim_time;

		// 대기하는 고객이 페이코적립고객인지 저장
		kiosk2_ispayco[num_in_kiosk2_q] = payco;

	}
	else {
		//키오스크 IDLE -> 딜레이 없이 바로 서비스 받음 
		kiosk2_delay = 0.0;
		total_of_kiosk2_delays += kiosk2_delay;
		//키오스크 에서 delay 마친 고객 1명 증가 & 키오스크 상태 BUSY로 
		++num_custs_kiosk2_delayed;
		kiosk2_status = BUSY;

		//kiosk2_depart 시각 설정 
		time_next_event[3] = sim_time + kiosk2_servicetime;
	}
}


void kiosk2_depart() {
	int i;
	float kiosk2_delay;
	float kiosk2_servicetime;

	if (num_in_kiosk2_q == 0) {
		//queue가 비어있으니까 키오스크 IDLE & 키오스크 출발 시각 무한 설정 
		kiosk2_status = IDLE;
		time_next_event[3] = 1.0e+30;
	}
	else {
		--num_in_kiosk2_q;
		//고객의 키오스크 delay 시간 & 누적 키오스크  delay 시간 계산
		kiosk2_delay = sim_time - time_kiosk2_arrival[1];


		total_of_kiosk2_delays += kiosk2_delay;

		if (max_kiosk2_delay <= kiosk2_delay) {
			max_kiosk2_delay = kiosk2_delay;
		}


		//키오스크 delay 시간이 3분(180초) 이상인 사람 수 조사

		if (kiosk2_delay >= 180) {
			++kiosk2_num_delay_over_3;
		}

		//키오스크 delay 시간이 4분(240초) 이상인 사람 수 조사
		if (kiosk2_delay >= 240) {
			++kiosk2_num_delay_over_4;
		}

		++num_custs_kiosk2_delayed;

		if (kiosk2_ispayco[1] == 1)
			kiosk2_servicetime = kiosk_service_AR(kiosk_gamma, kiosk_delta, kiosk_lambda, kiosk_zeta);
		else
			kiosk2_servicetime = normal_service(server_k, server_alpha, server_beta);
		time_next_event[3] = sim_time + kiosk2_servicetime;

		//고객 도착 시각 한 칸씩 앞당기기 
		for (i = 1; i <= num_in_kiosk2_q; ++i)
			time_kiosk2_arrival[i] = time_kiosk2_arrival[i + 1];
	}
}
void report() {
	fprintf(outfile, "\n\n\n*****************************************************************************************************************\n");
	fprintf(outfile, "OUTPUT 확인\n");
	fprintf(outfile, "\n\n키오스크1 QUEUE의 평균 delay 시간 \t\t\t\t\t %11.3f seconds\n\n", total_of_kiosk1_delays / num_custs_kiosk1_delayed);
	fprintf(outfile, "키오스크2 QUEUE의 평균 delay 시간 \t\t\t\t\t %11.3f seconds\n\n", total_of_kiosk2_delays / num_custs_kiosk2_delayed);
	fprintf(outfile, "전체 QUEUE의 평균 delay 시간 \t\t\t\t\t %11.3f seconds\n\n", (total_of_kiosk1_delays + total_of_kiosk2_delays) / (num_custs_kiosk1_delayed + num_custs_kiosk2_delayed));
	fprintf(outfile, "키오스크1 QUEUE의 최대 delay 시간 \t\t\t\t\t %11.3f seconds\n\n", max_kiosk1_delay);
	fprintf(outfile, "키오스크2 QUEUE의 최대 delay 시간 \t\t\t\t %11.3f seconds\n\n", max_kiosk2_delay);
	fprintf(outfile, "키오스크1 QUEUE의 평균 사람 수 \t\t\t\t\t\t %10.3f명\n\n", area_num_in_kiosk1_q / (sim_time));
	fprintf(outfile, "키오스크2 QUEUE의 평균 사람 수 \t\t\t\t\t %10.3f명\n\n", area_num_in_kiosk2_q / (sim_time));
	fprintf(outfile, "전체 QUEUE의 평균 사람 수 \t\t\t\t\t\t %10.3f명\n\n", (area_num_in_kiosk1_q + area_num_in_kiosk2_q) / (2 * sim_time));
	fprintf(outfile, "키오스크1 QUEUE의 최대 사람 수 \t\t\t\t\t\t %10.3d명 \n\n", max_num_in_kiosk1_q);
	fprintf(outfile, "키오스크2 QUEUE의 최대 사람 수 \t\t\t\t\t %10.3d명 \n\n", max_num_in_kiosk2_q);
	fprintf(outfile, "키오스크1 Utilization \t\t\t\t\t\t %15.3f\n\n", area_kiosk1_status / (sim_time ));
	fprintf(outfile, "키오스크2 Utilization \t\t\t\t\t %15.3f\n\n", area_kiosk2_status / (sim_time ));
	fprintf(outfile, "키오스크1 QUEUE에서 delay를 마친 고객 수 \t\t\t\t %d 명\n\n", num_custs_kiosk1_delayed);
	fprintf(outfile, "키오스크2 QUEUE에서 delay를 마친 고객 수 \t\t\t\t %d 명\n\n", num_custs_kiosk2_delayed);
	fprintf(outfile, "전체 system에서 delay를 마친 고객 수 \t\t\t\t %d 명\n\n", num_custs_kiosk1_delayed + num_custs_kiosk2_delayed);
	fprintf(outfile, "키오스크1에서 delay 시간이 3분(180초) 이상인 사람 수의 비율 \t\t\t %f %%\n\n", (float)(kiosk1_num_delay_over_3) / (float)(num_custs_kiosk1_delayed));
	fprintf(outfile, "키오스크1에서 delay 시간이 4분(240초) 이상인 사람 수의 비율 \t\t\t %f %%\n\n", (float)(kiosk1_num_delay_over_4) / (float)(num_custs_kiosk1_delayed));
	fprintf(outfile, "키오스크2에서 delay 시간이 3분(180초) 이상인 사람 수의 비율 \t\t %f %%\n\n", (float)(kiosk2_num_delay_over_3) / (float)(num_custs_kiosk2_delayed));
	fprintf(outfile, "키오스크2에서 delay 시간이 4분(240초) 이상인 사람 수의 비율 \t\t %f %%\n\n", (float)(kiosk2_num_delay_over_4) / (float)(num_custs_kiosk2_delayed));
	fprintf(outfile, "전체 시스템에서 3분 이상 기다린 사람의 비율 \t\t\t\t %f %% \n\n", (float)(kiosk1_num_delay_over_3 + kiosk2_num_delay_over_3) / (float)(num_custs_kiosk1_delayed + num_custs_kiosk2_delayed));
	fprintf(outfile, "전체 시스템에서 4분 이상 기다린 사람의 비율 \t\t\t\t %f %% \n\n", (float)(kiosk1_num_delay_over_4 + kiosk2_num_delay_over_4) / (float)(num_custs_kiosk1_delayed + num_custs_kiosk2_delayed));

}


//Random number generation

// payco 적립혜택을 받고자 하는 고객인지 여부
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

// 고객의 도착 난수 생성, Generalized Pareto 분포의 Inverse Transform 기법을 이용하였다.
float cust_arrival_pareto(float interarrival_k, float interarrival_sigma, float interarrival_mu) {

	float u, x;
	u = lcgrand(seed);
	x = (interarrival_sigma / interarrival_k) * (pow(u, -interarrival_k) - 1.0) + interarrival_mu;

	return x;
}

// payco 적립을 받지 않는 일반 결제 고객 서비스 시간 난수 생성 Dagum 분포의 Inverse Transform 기법을 이용하였다.
float normal_service(float server_k, float server_alpha, float server_beta) {
	float u, x;
	u = lcgrand(seed);
	x = server_beta * pow((pow((1.0 / u), 1 / server_k) - 1.0) , (1.0 / server_alpha));
	return x;
}

// payco 적립을 받는 결제 고객 서비스 시간 난수 생성, Johnson SB 분포를 따르며, Acceptance-Rejection 기법을 이용하였다.
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



