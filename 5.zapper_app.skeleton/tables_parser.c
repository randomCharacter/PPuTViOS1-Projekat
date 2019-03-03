#include "tables.h"

ParseErrorCode parsePatHeader(const uint8_t *pat_header_buffer, PatHeader *pat_header)
{
	if (pat_header_buffer == NULL || pat_header == NULL)
	{
		printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	pat_header->tableId = (uint8_t)*pat_header_buffer;
	if (pat_header->tableId != 0x00)
	{
		printf("\n%s : ERROR it is not a PAT Table\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	uint8_t lower_8_bits = 0;
	uint8_t higher_8_bits = 0;
	uint16_t all_16_bits = 0;

	lower_8_bits = (uint8_t)(*(pat_header_buffer + 1));
	lower_8_bits = lower_8_bits >> 7;
	pat_header->sectionSyntaxIndicator = lower_8_bits & 0x01;

	higher_8_bits = (uint8_t)(*(pat_header_buffer + 1));
	lower_8_bits = (uint8_t)(*(pat_header_buffer + 2));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	pat_header->sectionLength = all_16_bits & 0x0FFF;

	higher_8_bits = (uint8_t)(*(pat_header_buffer + 3));
	lower_8_bits = (uint8_t)(*(pat_header_buffer + 4));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	pat_header->transportStreamId = all_16_bits & 0xFFFF;

	lower_8_bits = (uint8_t)(*(pat_header_buffer + 5));
	lower_8_bits = lower_8_bits >> 1;
	pat_header->versionNumber = lower_8_bits & 0x1F;

	lower_8_bits = (uint8_t)(*(pat_header_buffer + 5));
	pat_header->currentNextIndicator = lower_8_bits & 0x01;

	lower_8_bits = (uint8_t)(*(pat_header_buffer + 6));
	pat_header->sectionNumber = lower_8_bits & 0xFF;

	lower_8_bits = (uint8_t)(*(pat_header_buffer + 7));
	pat_header->lastSectionNumber = lower_8_bits & 0xFF;

	return TABLES_PARSE_OK;
}

ParseErrorCode parsePatServiceInfo(const uint8_t *pat_service_info_buffer, PatServiceInfo *pat_service_info)
{
	if (pat_service_info_buffer == NULL || pat_service_info == NULL)
	{
		printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	uint8_t lower_8_bits = 0;
	uint8_t higher_8_bits = 0;
	uint16_t all_16_bits = 0;

	higher_8_bits = (uint8_t)(*(pat_service_info_buffer));
	lower_8_bits = (uint8_t)(*(pat_service_info_buffer + 1));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	pat_service_info->programNumber = all_16_bits & 0xFFFF;

	higher_8_bits = (uint8_t)(*(pat_service_info_buffer + 2));
	lower_8_bits = (uint8_t)(*(pat_service_info_buffer + 3));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	pat_service_info->pid = all_16_bits & 0x1FFF;

	return TABLES_PARSE_OK;
}

ParseErrorCode parsePatTable(const uint8_t *pat_section_buffer, PatTable *pat_table)
{
	uint8_t *current_buffer_position = NULL;
	uint32_t parsed_length = 0;

	if (pat_section_buffer == NULL || pat_table == NULL)
	{
		printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	if (parsePatHeader(pat_section_buffer, &(pat_table->patHeader)) != TABLES_PARSE_OK)
	{
		printf("\n%s : ERROR parsing PAT header\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	parsed_length = 12 /*PAT header size*/ - 3 /*Not in section length*/;
	current_buffer_position = (uint8_t *)(pat_section_buffer + 8); /* Position after last_section_number */
	pat_table->serviceInfoCount = 0;							   /* Number of services info presented in PAT table */

	while (parsed_length < pat_table->patHeader.sectionLength)
	{
		if (pat_table->serviceInfoCount > TABLES_MAX_NUMBER_OF_PIDS_IN_PAT - 1)
		{
			printf("\n%s : ERROR there is not enough space in PAT structure for Service info\n", __FUNCTION__);
			return TABLES_PARSE_ERROR;
		}

		if (parsePatServiceInfo(current_buffer_position, &(pat_table->patServiceInfoArray[pat_table->serviceInfoCount])) == TABLES_PARSE_OK)
		{
			current_buffer_position += 4; /* Size from program_number to pid */
			parsed_length += 4;			  /* Size from program_number to pid */
			pat_table->serviceInfoCount++;
		}
	}

	return TABLES_PARSE_OK;
}

ParseErrorCode printPatTable(PatTable *pat_table)
{
	uint8_t i = 0;

	if (pat_table == NULL)
	{
		printf("\n%s : ERROR received parameter is not ok\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	printf("\n********************PAT TABLE SECTION********************\n");
	printf("table_id                 |      %d\n", pat_table->patHeader.tableId);
	printf("section_length           |      %d\n", pat_table->patHeader.sectionLength);
	printf("transport_stream_id      |      %d\n", pat_table->patHeader.transportStreamId);
	printf("section_number           |      %d\n", pat_table->patHeader.sectionNumber);
	printf("last_section_number      |      %d\n", pat_table->patHeader.lastSectionNumber);

	for (i = 0; i < pat_table->serviceInfoCount; i++)
	{
		printf("-----------------------------------------\n");
		printf("program_number           |      %d\n", pat_table->patServiceInfoArray[i].programNumber);
		printf("pid                      |      %d\n", pat_table->patServiceInfoArray[i].pid);
	}
	printf("\n********************PAT TABLE SECTION********************\n");

	return TABLES_PARSE_OK;
}

ParseErrorCode parsePmtHeader(const uint8_t *pmt_header_buffer, PmtTableHeader *pmt_header)
{

	if (pmt_header_buffer == NULL || pmt_header == NULL)
	{
		printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	pmt_header->tableId = (uint8_t)*pmt_header_buffer;
	if (pmt_header->tableId != 0x02)
	{
		printf("\n%s : ERROR it is not a PMT Table\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	uint8_t lower_8_bits = 0;
	uint8_t higher_8_bits = 0;
	uint16_t all_16_bits = 0;

	lower_8_bits = (uint8_t)(*(pmt_header_buffer + 1));
	lower_8_bits = lower_8_bits >> 7;
	pmt_header->sectionSyntaxIndicator = lower_8_bits & 0x01;

	higher_8_bits = (uint8_t)(*(pmt_header_buffer + 1));
	lower_8_bits = (uint8_t)(*(pmt_header_buffer + 2));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	pmt_header->sectionLength = all_16_bits & 0x0FFF;

	higher_8_bits = (uint8_t)(*(pmt_header_buffer + 3));
	lower_8_bits = (uint8_t)(*(pmt_header_buffer + 4));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	pmt_header->programNumber = all_16_bits & 0xFFFF;

	lower_8_bits = (uint8_t)(*(pmt_header_buffer + 5));
	lower_8_bits = lower_8_bits >> 1;
	pmt_header->versionNumber = lower_8_bits & 0x1F;

	lower_8_bits = (uint8_t)(*(pmt_header_buffer + 5));
	pmt_header->currentNextIndicator = lower_8_bits & 0x01;

	lower_8_bits = (uint8_t)(*(pmt_header_buffer + 6));
	pmt_header->sectionNumber = lower_8_bits & 0xFF;

	lower_8_bits = (uint8_t)(*(pmt_header_buffer + 7));
	pmt_header->lastSectionNumber = lower_8_bits & 0xFF;

	higher_8_bits = (uint8_t)(*(pmt_header_buffer + 8));
	lower_8_bits = (uint8_t)(*(pmt_header_buffer + 9));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	pmt_header->pcrPid = all_16_bits & 0xFFFF;

	higher_8_bits = (uint8_t)(*(pmt_header_buffer + 10));
	lower_8_bits = (uint8_t)(*(pmt_header_buffer + 11));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	pmt_header->programInfoLength = all_16_bits & 0x0FFF;

	return TABLES_PARSE_OK;
}

ParseErrorCode parsePmtElementaryInfo(const uint8_t *pmt_elementary_info_buffer, PmtElementaryInfo *pmt_elementary_info)
{
	if (pmt_elementary_info_buffer == NULL || pmt_elementary_info == NULL)
	{
		printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	uint8_t lower_8_bits = 0;
	uint8_t higher_8_bits = 0;
	uint16_t all_16_bits = 0;

	pmt_elementary_info->streamType = *pmt_elementary_info_buffer;

	higher_8_bits = (uint8_t)(*(pmt_elementary_info_buffer + 1));
	lower_8_bits = (uint8_t)(*(pmt_elementary_info_buffer + 2));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	pmt_elementary_info->elementaryPid = all_16_bits & 0x1FFF;

	higher_8_bits = (uint8_t)(*(pmt_elementary_info_buffer + 3));
	lower_8_bits = (uint8_t)(*(pmt_elementary_info_buffer + 4));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	pmt_elementary_info->esInfoLength = all_16_bits & 0x0FFF;

	return TABLES_PARSE_OK;
}

ParseErrorCode parsePmtTable(const uint8_t *pmt_section_buffer, PmtTable *pmt_table)
{
	uint8_t *current_buffer_position = NULL;
	uint32_t parsed_length = 0;

	if (pmt_section_buffer == NULL || pmt_table == NULL)
	{
		printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	if (parsePmtHeader(pmt_section_buffer, &(pmt_table->pmtHeader)) != TABLES_PARSE_OK)
	{
		printf("\n%s : ERROR parsing PMT header\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	parsed_length = 12 + pmt_table->pmtHeader.programInfoLength /*PMT header size*/ + 4 /*CRC size*/ - 3 /*Not in section length*/;
	current_buffer_position = (uint8_t *)(pmt_section_buffer + 12 + pmt_table->pmtHeader.programInfoLength); /* Position after last descriptor */
	pmt_table->elementaryInfoCount = 0;																		 /* Number of elementary info presented in PMT table */

	while (parsed_length < pmt_table->pmtHeader.sectionLength)
	{
		if (pmt_table->elementaryInfoCount > TABLES_MAX_NUMBER_OF_ELEMENTARY_PID - 1)
		{
			printf("\n%s : ERROR there is not enough space in PMT structure for elementary info\n", __FUNCTION__);
			return TABLES_PARSE_ERROR;
		}

		if (parsePmtElementaryInfo(current_buffer_position, &(pmt_table->pmtElementaryInfoArray[pmt_table->elementaryInfoCount])) == TABLES_PARSE_OK)
		{
			current_buffer_position += 5 + pmt_table->pmtElementaryInfoArray[pmt_table->elementaryInfoCount].esInfoLength; /* Size from stream type to elemntary info descriptor*/
			parsed_length += 5 + pmt_table->pmtElementaryInfoArray[pmt_table->elementaryInfoCount].esInfoLength;		   /* Size from stream type to elementary info descriptor */
			pmt_table->elementaryInfoCount++;
		}
	}

	return TABLES_PARSE_OK;
}

ParseErrorCode printPmtTable(PmtTable *pmt_table)
{
	uint8_t i = 0;

	if (pmt_table == NULL)
	{
		printf("\n%s : ERROR received parameter is not ok\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	printf("\n********************PMT TABLE SECTION********************\n");
	printf("table_id                 |      %d\n", pmt_table->pmtHeader.tableId);
	printf("section_length           |      %d\n", pmt_table->pmtHeader.sectionLength);
	printf("program_number           |      %d\n", pmt_table->pmtHeader.programNumber);
	printf("section_number           |      %d\n", pmt_table->pmtHeader.sectionNumber);
	printf("last_section_number      |      %d\n", pmt_table->pmtHeader.lastSectionNumber);
	printf("program_info_legth       |      %d\n", pmt_table->pmtHeader.programInfoLength);

	for (i = 0; i < pmt_table->elementaryInfoCount; i++)
	{
		printf("-----------------------------------------\n");
		printf("stream_type              |      %d\n", pmt_table->pmtElementaryInfoArray[i].streamType);
		printf("elementary_pid           |      %d\n", pmt_table->pmtElementaryInfoArray[i].elementaryPid);
	}
	printf("\n********************PMT TABLE SECTION********************\n");

	return TABLES_PARSE_OK;
}

ParseErrorCode parseEitHeader(const uint8_t *eit_header_buffer, EitTableHeader *eit_header)
{
	if (eit_header_buffer == NULL || eit_header == NULL)
	{
		printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	eit_header->tableId = (uint8_t)*eit_header_buffer;
	if (eit_header->tableId != 0x4E)
	{
		printf("\n%s : ERROR it is not a EIT Table\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	uint8_t lower_8_bits = 0;
	uint8_t higher_8_bits = 0;
	uint16_t all_16_bits = 0;

	lower_8_bits = (uint8_t)(*(eit_header_buffer + 1));
	lower_8_bits = lower_8_bits >> 7;
	eit_header->sectionSyntaxIndicator = lower_8_bits & 0x01;

	lower_8_bits = (uint8_t)(*(eit_header_buffer + 1));
	lower_8_bits = lower_8_bits >> 6;
	eit_header->reservedFutureUse = lower_8_bits & 0x01;

	lower_8_bits = (uint8_t)(*(eit_header_buffer + 1));
	lower_8_bits = lower_8_bits >> 4;
	eit_header->reserved1 = lower_8_bits & 0x03;

	higher_8_bits = (uint8_t)(*(eit_header_buffer + 1));
	lower_8_bits = (uint8_t)(*(eit_header_buffer + 2));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	eit_header->sectionLength = all_16_bits & 0x0FFF;

	higher_8_bits = (uint8_t)(*(eit_header_buffer + 3));
	lower_8_bits = (uint8_t)(*(eit_header_buffer + 4));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	eit_header->serviceId = all_16_bits;

	lower_8_bits = (uint8_t)(*(eit_header_buffer + 5));
	lower_8_bits = lower_8_bits >> 6;
	eit_header->reserved2 = lower_8_bits & 0x03;

	lower_8_bits = (uint8_t)(*(eit_header_buffer + 5));
	lower_8_bits = lower_8_bits >> 1;
	eit_header->versionNumber = lower_8_bits & 0x1F;

	lower_8_bits = (uint8_t)(*(eit_header_buffer + 5));
	eit_header->currentNextIndicator = lower_8_bits & 0x01;

	lower_8_bits = (uint8_t)(*(eit_header_buffer + 6));
	eit_header->sectionNumber = lower_8_bits;

	lower_8_bits = (uint8_t)(*(eit_header_buffer + 7));
	eit_header->lastSectionNumber = lower_8_bits;

	higher_8_bits = (uint8_t)(*(eit_header_buffer + 8));
	lower_8_bits = (uint8_t)(*(eit_header_buffer + 9));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	eit_header->transportStreamId = all_16_bits;

	higher_8_bits = (uint8_t)(*(eit_header_buffer) + 10);
	lower_8_bits = (uint8_t)(*(eit_header_buffer) + 11);
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	eit_header->originalNetworkId = all_16_bits;

	lower_8_bits = (uint8_t)(*(eit_header_buffer + 12));
	eit_header->segmentLastSectionNumber = lower_8_bits;

	lower_8_bits = (uint8_t)(*(eit_header_buffer + 13));
	eit_header->lastTableId = lower_8_bits;

	return TABLES_PARSE_OK;
}

ParseErrorCode parseEitTableInfo(const uint8_t *eit_info_buffer, EitTableInfo *eit_info)
{
	int i;
	int j;
	int k;

	if (eit_info_buffer == NULL || eit_info == NULL)
	{
		printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	uint8_t lower_8_bits = 0;
	uint8_t higher_8_bits = 0;
	uint16_t all_16_bits = 0;

	uint64_t timeResult = 0;
	uint32_t durationResult = 0;
	uint8_t part0 = 0;
	uint8_t part1 = 0;
	uint8_t part2 = 0;
	uint8_t part3 = 0;
	uint8_t part4 = 0;

	higher_8_bits = (uint8_t)(*(eit_info_buffer));
	lower_8_bits = (uint8_t)(*(eit_info_buffer + 1));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	eit_info->eventId = all_16_bits;

	part4 = (uint8_t)(*(eit_info_buffer + 2));
	part3 = (uint8_t)(*(eit_info_buffer + 3));
	part2 = (uint8_t)(*(eit_info_buffer + 4));
	part1 = (uint8_t)(*(eit_info_buffer + 5));
	part0 = (uint8_t)(*(eit_info_buffer + 6));
	timeResult = (((uint64_t)part4 << 32) + (part3 << 24) + (part2 << 16) + (part1 << 8) + part0);
	eit_info->startTime = timeResult;

	part2 = (uint8_t)(*(eit_info_buffer + 7));
	part1 = (uint8_t)(*(eit_info_buffer + 8));
	part0 = (uint8_t)(*(eit_info_buffer + 9));
	durationResult = (uint32_t)((part2 << 16) + (part1 << 8) + part0);
	eit_info->duration = durationResult;

	lower_8_bits = (uint8_t)(*(eit_info_buffer + 10));
	lower_8_bits = lower_8_bits >> 5;
	eit_info->runningStatus = lower_8_bits & 0x07;

	lower_8_bits = (uint8_t)(*(eit_info_buffer + 10));
	lower_8_bits = lower_8_bits >> 4;
	eit_info->freeCAmode = lower_8_bits & 0x01;

	higher_8_bits = (uint8_t)(*(eit_info_buffer + 10));
	lower_8_bits = (uint8_t)(*(eit_info_buffer + 11));
	all_16_bits = (uint16_t)((higher_8_bits << 8) + lower_8_bits);
	eit_info->descriptorsLoopLength = all_16_bits & 0x0FFF;

	k = 0;

	while (k < eit_info->descriptorsLoopLength)
	{
		eit_info->descriptor.descriptorTag = *(eit_info_buffer + 12 + k);
		eit_info->descriptor.descriptorLength = *(eit_info_buffer + 12 + 1 + k);

		if (eit_info->descriptor.descriptorTag == 0x4d)
		{
			//name length of event
			eit_info->descriptor.eventNameLength = *(eit_info_buffer + 12 + 5 + k);
			//getting all the chars of event
			for (i = 0; i < eit_info->descriptor.eventNameLength; i++)
			{
				eit_info->descriptor.eventNameChar[i] = (char)(*(eit_info_buffer + 12 + 6 + i + k + 1));
			}
			//setting end of event chars
			eit_info->descriptor.eventNameChar[eit_info->descriptor.eventNameLength - 1] = '\0';

			//description of emision that is streamed
			eit_info->descriptor.descriptionLength = *(eit_info_buffer + 12 + 6 + eit_info->descriptor.eventNameLength + k);
			//getting chars
			for (j = 0; j < eit_info->descriptor.descriptionLength; j++)
			{
				eit_info->descriptor.descriptionChar[j] = (char)(*(eit_info_buffer + 12 + 6 + eit_info->descriptor.eventNameLength + 1 + j + k + 1));
			}
			//end of string
			eit_info->descriptor.descriptionChar[eit_info->descriptor.descriptionLength - 1] = '\0';
		}
		k += eit_info->descriptor.descriptorLength + 2;
	}

	return TABLES_PARSE_OK;
}

ParseErrorCode parseEitTable(const uint8_t *eit_section_buffer, EitTable *eit_table)
{
	uint8_t *current_buffer_position = NULL;
	uint32_t parsed_length = 0;

	if (eit_section_buffer == NULL || eit_table == NULL)
	{
		printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	if (parseEitHeader(eit_section_buffer, &(eit_table->eitHeader)) != TABLES_PARSE_OK)
	{
		printf("\n%s : ERROR parsing EIT header\n", __FUNCTION__);
		return TABLES_PARSE_ERROR;
	}

	parsed_length = 14 /*EIT header size*/ + 4 /*CRC size*/ - 3 /*Not in section length*/;
	current_buffer_position = (uint8_t *)(eit_section_buffer + 14); /*Position after reserved_future_use*/
	eit_table->eventsInfoCount = 0;									/* Number of elementary info presented in EIT table */

	while (parsed_length < eit_table->eitHeader.sectionLength)
	{
		if (eit_table->eventsInfoCount > TABLES_MAX_NUMBER_OF_EIT_PID - 1)
		{
			printf("\n%s : ERROR there is not enough space in EIT structure for elementary info\n", __FUNCTION__);
			return TABLES_PARSE_ERROR;
		}
		if (parseEitTableInfo(current_buffer_position, &(eit_table->eitInfoArray[eit_table->eventsInfoCount])) == TABLES_PARSE_OK)
		{
			current_buffer_position += 12 + eit_table->eitInfoArray[eit_table->eventsInfoCount].descriptorsLoopLength; // Size from stream type to elemntary info descriptor
			parsed_length += 12 + eit_table->eitInfoArray[eit_table->eventsInfoCount].descriptorsLoopLength;		   // Size from stream type to elementary info descriptor
			eit_table->eventsInfoCount++;
		}
	}

	return TABLES_PARSE_OK;
}
